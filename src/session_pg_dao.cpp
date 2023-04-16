// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022 Frank Dean <frank.dean@fdsd.co.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "../config.h"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/dao_helper.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include <iostream>
#include <sstream>

using namespace pqxx;
using namespace fdsd::utils;
using namespace fdsd::web;
using namespace fdsd::trip;

const std::string SessionPgDao::coordinate_format_key = "coordinate_format";
const std::string SessionPgDao::tracks_query_key = "tracks";
/// Used for copy-and-paste of location tracking history
const std::string SessionPgDao::location_history_key = "location-history";
/// Used for copy-and-paste of itinerary featues
const std::string SessionPgDao::itinerary_features_key = "itinerary-features";
/// Used to keep the last viewed page of itineraries
const std::string SessionPgDao::itinerary_page_key = "itineraries";
/// Used to keep the last viewed page of itinerary radius search results
const std::string SessionPgDao::itinerary_radius_search_page_key = "itinerary-search-page-key";

const std::string SessionPgDao::insert_session_ps_name = "session_insert";

const std::string SessionPgDao::insert_session_sql =
  "INSERT INTO session (id, user_id, updated) "
  "VALUES ($1, $2, to_timestamp($3)) "
  "ON CONFLICT (id) DO UPDATE SET user_id=$2, updated=to_timestamp($3)";

const std::string SessionPgDao::create_session_table_sql =
  "CREATE TABLE IF NOT EXISTS session ("
  "id UUID PRIMARY KEY, "
  "user_id integer NOT NULL, "
  "updated timestamp WITHOUT TIME ZONE NOT NULL, "
  "FOREIGN KEY (user_id) "
  "REFERENCES usertable (id) ON DELETE CASCADE"
  ")";

const std::string SessionPgDao::create_session_data_table_sql =
  "CREATE TABLE IF NOT EXISTS session_data ("
  "session_id uuid NOT NULL,"
  "key text NOT NULL,"
  "value text,"
  "PRIMARY KEY (session_id, key),"
  "FOREIGN KEY (session_id) "
  "REFERENCES session (id) ON DELETE CASCADE)";

void SessionPgDao::save_value(std::string session_id,
                              std::string key,
                              std::string value)
{
  work tx(*connection);
  tx.exec(
      "INSERT INTO session_data (session_id, key, value) VALUES ('" +
      tx.esc(session_id) + "', '" + tx.esc(key) + "', '" + tx.esc(value) + "') "
      "ON CONFLICT (session_id, key) DO UPDATE SET key='" +
      tx.esc(key) + "', value='" + tx.esc(value) + "'"
    );
  tx.commit();
}

std::string SessionPgDao::get_value(std::string session_id,
                                    std::string key)
{
  work tx(*connection);
  result r = tx.exec(
      "SELECT value FROM session_data "
      "WHERE session_id='" + tx.esc(session_id) +
      "' AND key = '" + tx.esc(key) + "'");
  tx.commit();
  return r.empty() ? "" : r[0][0].as<std::string>();
}

void SessionPgDao::remove_value(std::string session_id,
                                std::string key)
{
  work tx(*connection);
  result r = tx.exec(
      "DELETE FROM session_data "
      "WHERE session_id='" + tx.esc(session_id) +
      "' AND key = '" + tx.esc(key) + "'");
  tx.commit();
}

void SessionPgDao::create_session_table(bool overwrite)
{
  work tx(*connection);
  try {
    tx.exec(create_session_table_sql);
    tx.commit();
  } catch (const std::exception& e) {
    std::cerr << "Failure creating session table: "
              << e.what() << '\n';
    throw;
  }
}

void SessionPgDao::save_session(std::string session_id,
                                const fdsd::web::Session session)
{
  connection->prepare(
      insert_session_ps_name,
      insert_session_sql
    );
  work tx(*connection);
  tx.exec_prepared(insert_session_ps_name,
                   session_id,
                   session.get_user_id(),
                   std::chrono::system_clock::to_time_t(
                       session.get_last_updated_time_point()));
  tx.commit();
}

void SessionPgDao::save_sessions(const session_map sessions)
{
  connection->prepare(
      insert_session_ps_name,
      insert_session_sql
    );
  work tx(*connection);
  for (const auto session : sessions) {
    tx.exec_prepared(insert_session_ps_name,
                     session.first,
                     session.second.get_user_id(),
                     std::chrono::system_clock::to_time_t(
                         session.second.get_last_updated_time_point()));
  }
  const int max_session_minutes = SessionManager::get_session_manager()->
    get_max_session_minutes();
  tx.exec("DELETE FROM session WHERE updated < current_timestamp - INTERVAL '" +
          std::to_string(max_session_minutes) + " minutes'");
  tx.commit();
}

void SessionPgDao::delete_all_sessions()
{
  work tx(*connection);
  tx.exec("DELETE FROM session");
  tx.commit();
}

void SessionPgDao::load_sessions(session_map &sessions) {
  work tx(*connection);
  result r = tx.exec(
      "SELECT id, user_id, updated FROM session"
    );
  for (result::const_iterator i = r.begin(); i != r.end(); ++i) {
    // std::string s1 = ((*i)[0]).c_str();
    // std::string s2 = ((*i)[1]).c_str();
    std::string user_id;
    i["user_id"].to(user_id);
    Session s(user_id);
    std::string session_id;
    i["id"].to(session_id);
    std::string time;
    i["updated"].to(time);
    s.set_date(time);
    sessions[session_id] = s;
  }
  tx.commit();
}

void SessionPgDao::invalidate_session(const std::string session_id)
{
  work tx(*connection);
  result r =
    tx.exec("DELETE FROM session WHERE id='" + tx.esc(session_id) + "'");
  tx.commit();
}

bool SessionPgDao::validate_password(std::string email, std::string password)
{
  bool retval = false;
  work tx(*connection);

  // https://stackoverflow.com/questions/55557494/use-pgcrypto-to-verify-passwords-generated-by-password-hash
  // https://stackoverflow.com/questions/15733196/where-2x-prefix-are-used-in-bcrypt

  // So, when the Blowfish algorithm changes, it has a different 3 character
  // prefix, e.g. '$2y', '$2b'.  At the time of writing, it's '$3a'.
  // Apparently, it shouldn't matter which prefix is used, but we see
  // different results with prefixes other than '$2a'.  The fix seems simply
  // to update the prefix strings in the database (after a backup!).
  //
  // UPDATE usertable SET password='$2a' || SUBSTRING(password, 4, 57) WHERE password NOT LIKE '$2a%';

  // We could also ignore the prefix in the checks, but this doesn't seem very future proof, e.g.
  //
  // const std::string blowfish_prefix = "$2a";
  // result result = tx.exec(
  //     "SELECT ('$2a' || substring(u.password, 4, 57) = "
  //     "crypt('" + tx.esc(password) + "', '$2a' || substring(u.password, 4, 57))) "
  //     "AS pswmatch FROM "
  //     "(SELECT password FROM usertable WHERE email='" + tx.esc(email) + "') as u"
  //   );

  result result = tx.exec(
      "SELECT (u.password = crypt('" + tx.esc(password) + "', u.password)) "
      "AS pswmatch FROM "
      "(SELECT password FROM usertable WHERE email='" + tx.esc(email) + "') as u"
    );

  if (!result.empty()) {
    result[0][0].to(retval);
  }
  tx.commit();
  return retval;
}

bool SessionPgDao::validate_password_by_user_id(
    std::string user_id, std::string password)
{
  try {
    bool retval = false;
    work tx(*connection);
    auto r = tx.exec_params1(
        "SELECT (u.password = crypt($2, u.password)) "
        "AS pswmatch FROM "
        "(SELECT password FROM usertable WHERE id=$1) as u",
        user_id,
        password
      );
    r[0].to(retval);
    tx.commit();
    return retval;
  } catch (const std::exception &e) {
    std::cerr << "Error validating password by user_id: "
              << e.what() << std::endl;
    throw;
  }
}

void SessionPgDao::change_password(std::string user_id,
                                   std::string new_password)
{
  try {
    work tx(*connection);
    // https://www.postgresql.org/docs/13/pgcrypto.html
    tx.exec_params("UPDATE usertable SET password=crypt($2, gen_salt('bf')) "
                   "WHERE id=$1",
                   user_id,
                   new_password);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Error changing password: " << e.what() << std::endl;
    throw;
  }
}

std::string SessionPgDao::get_user_id_by_email(const std::string email)
{
  std::string user_id;
  work tx(*connection);
  result r = tx.exec(
      "SELECT (id) FROM usertable WHERE email = '" +
      tx.esc(email) + "'");
  if (!r.empty())
    r[0][0].to(user_id);
  tx.commit();
  return user_id;
}

bool SessionPgDao::is_admin(std::string user_id)
{
  if (user_id.empty())
    return false;

  work tx(*connection);
  result r = tx.exec(
      "SELECT count(*) "
      "FROM user_role ur JOIN role r ON ur.role_id=id "
      "WHERE r.name='Admin' AND ur.user_id='" + tx.esc(user_id) + '\'');
  tx.commit();
  return !r.empty() && r[0][0].as<int>() > 0;
}

void SessionPgDao::upgrade()
{
  try {
    work tx(*connection);
    tx.exec(create_session_table_sql);
    tx.exec(create_session_data_table_sql);
    // tx.exec("ALTER TABLE session_data ADD PRIMARY KEY (session_id, key)");

    // Fix old passwords, see above comments re Blowfish algorithm changes.
    tx.exec("UPDATE usertable SET password = '$2a' || SUBSTRING(password, 4, 57) WHERE password NOT LIKE '$2a%'");
    tx.commit();
  } catch (const std::exception& e) {
    std::cerr << "Failure creating session table: "
              << e.what() << '\n';
    throw;
  }

  try {
    work tx(*connection);
    tx.exec("CREATE EXTENSION pgcrypto");
    tx.commit();
  } catch (const std::exception& e) {
    std::cerr << "Failure creating pgcrypto extension: "
              << e.what() << '\n';
  }
}

SessionPgDao::tile_report
    SessionPgDao::get_tile_usage_metrics(int month_count)
{
  try {
    work tx(*connection);
    tile_report report;
    auto summary = tx.exec_params1(
        "SELECT time, count FROM tile_metric ORDER BY time DESC LIMIT 1");
    std::string date_str;
    report.time.first = summary["time"].to(date_str);
    report.time.second = dao_helper::convert_libpq_date_tz(date_str);
    report.total.first = summary["count"].to(report.total.second);
    auto result = tx.exec_params(
        "SELECT year, month, max(count) AS cumulative_total FROM ("
        "SELECT time, extract(year from time) AS year, "
        "extract(month from time) AS month, "
        "extract(day from time) AS day, "
        "count FROM tile_metric ORDER BY time DESC) AS q "
        "GROUP BY q.year, q.month ORDER BY q.year desc, q.month DESC LIMIT $1",
        month_count);
    
    for (auto r: result) {
      SessionPgDao::tile_usage_metric t;
      r["year"].to(t.year);
      r["month"].to(t.month);
      r["cumulative_total"].to(t.cumulative_total);
      report.metrics.push_back(t);
    }
    auto previous = report.metrics.rend();
    for (auto t = report.metrics.rbegin(); t != report.metrics.rend(); t++) {
      if (previous != report.metrics.rend())
        t->quantity = t->cumulative_total - previous->cumulative_total;
      else
        t->quantity = t->cumulative_total;
      // std::cout << t->year << " " << t->month << " " << t->cumulative_total << " " << t->quantity << '\n';
      previous = t;
    }
    return report;
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Error getting tile usage metrics: " << e.what() << '\n';
    throw;
  }
}

void SessionPgDao::append_user_search_where_clause(
    work &tx,
    std::string email,
    std::string nickname,
    SessionPgDao::search_type search_type,
    std::ostream &sql)
{
  if (!email.empty() || !nickname.empty())
    sql << "WHERE ";
  if (!email.empty()) {
    switch (search_type) {
      case exact:
        sql << "email='" << tx.esc(email) << "' ";
        break;
      case partial:
        sql << "email LIKE '%" << tx.esc(email) << "%' ";
        break;
    }
  }
  if (!email.empty() && !nickname.empty())
    sql << " AND ";
  if (!nickname.empty()) {
    switch (search_type) {
      case exact:
        sql << "nickname='" << tx.esc(nickname) << "' ";
        break;
      case partial:
        sql << "nickname LIKE '%" << tx.esc(nickname) << "%' ";
        break;
    }
  }
}

long SessionPgDao::get_search_users_by_nickname_count(
    std::string email,
    std::string nickname,
    SessionPgDao::search_type search_type)
{
  try {
    work tx(*connection);
    std::ostringstream sql;
    sql <<
      "SELECT COUNT(*) "
      "FROM usertable u ";
    append_user_search_where_clause(tx, email, nickname, search_type, sql);
    auto r = tx.exec_params1(sql.str());
    tx.commit();
    return r[0].as<long>();
  } catch (const std::exception &e) {
    std::cerr << "Error getting count for searching users by nickname: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<SessionPgDao::user> SessionPgDao::search_users_by_nickname(
    std::string email,
    std::string nickname,
    SessionPgDao::search_type search_type,
    std::uint32_t offset,
    int limit)
{
  try {
    work tx(*connection);
    std::ostringstream sql;
    sql <<
      "SELECT DISTINCT ON (nickname) u.id, firstname, lastname, email, uuid, nickname, "
      "r.name='Admin' AND r.name IS NOT NULL AS admin "
      "FROM usertable u "
      "LEFT JOIN user_role ur ON ur.user_id=u.id LEFT JOIN role r on ur.role_id=r.id ";
    append_user_search_where_clause(tx, email, nickname, search_type, sql);
    sql << "ORDER BY nickname, admin DESC OFFSET $1 LIMIT $2";
    // std::cout << "SQL: " << sql.str() << '\n';
    auto result = tx.exec_params(
        sql.str(),
        offset,
        limit);
    std::vector<user> users;
    for (const auto r : result) {
      user u;
      u.id.first = r["id"].to(u.id.second);
      r["firstname"].to(u.firstname);
      r["lastname"].to(u.lastname);
      r["email"].to(u.email);
      u.uuid.first = r["uuid"].to(u.uuid.second);
      r["nickname"].to(u.nickname);
      r["admin"].to(u.is_admin);
      users.push_back(u);
    }
    tx.commit();
    return users;
  } catch (const std::exception &e) {
    std::cerr << "Error searching users by nickname: "
              << e.what() << '\n';
    throw;
  }
}

SessionPgDao::user SessionPgDao::get_user_details_by_user_id(
    std::string user_id)
{
  try {
    work tx(*connection);
    const std::string sql = // "SELECT id, firstname, lastname, email, uuid, nickname FROM usertable WHERE id=$1";
// SELECT DISTINCT ON (u.id)  u.id, firstname, lastname, email, uuid, nickname, r.name='Admin' AND r.name IS NOT NULL AS admin FROM usertable u LEFT JOIN user_role ur ON ur.user_id=u.id LEFT JOIN role r on ur.role_id=r.id WHERE u.id=6 ORDER BY u.id, admin DESC;
      "SELECT DISTINCT ON (u.id) u.id, firstname, lastname, email, uuid, nickname, "
      "r.name='Admin' AND r.name IS NOT NULL AS admin "
      "FROM usertable u "
      "LEFT JOIN user_role ur ON ur.user_id=u.id LEFT JOIN role r on ur.role_id=r.id "
      "WHERE u.id=$1 "
      "ORDER BY u.id, admin DESC";

    // std::cout << "Searching for user_id: \"" << user_id << "\"\nSQL: " << sql << '\n';
    auto r = tx.exec_params1(
        sql,
        user_id);
    user u;
    u.id.first = r["id"].to(u.id.second);
    r["firstname"].to(u.firstname);
    r["lastname"].to(u.lastname);
    r["email"].to(u.email);
    u.uuid.first = r["uuid"].to(u.uuid.second);
    r["nickname"].to(u.nickname);
    r["admin"].to(u.is_admin);
    tx.commit();
    return u;
  } catch (const std::exception &e) {
    std::cerr << "Error fetching user details by user_id: "
              << e.what() << '\n';
    throw;
  }
}

long SessionPgDao::save(const SessionPgDao::user &user_details)
{
  try {
    work tx(*connection);
    long id;
    if (user_details.id.first) {
      id = user_details.id.second;
      if (user_details.password.first) {
        tx.exec_params(
            "UPDATE usertable "
            "SET firstname=$2, lastname=$3, email=$4, "
            "nickname=$5, "
            "password=crypt($6, gen_salt('bf')) "
            "WHERE id=$1",
            user_details.id.second,
            user_details.firstname,
            user_details.lastname,
            user_details.email,
            user_details.nickname,
            user_details.password.first ? &user_details.password.second : nullptr
          );
      } else {
        tx.exec_params(
            "UPDATE usertable "
            "SET firstname=$2, lastname=$3, email=$4, "
            "nickname=$5 "
            "WHERE id=$1",
            user_details.id.second,
            user_details.firstname,
            user_details.lastname,
            user_details.email,
            user_details.nickname
          );
      }
    } else {
      auto r = tx.exec_params1(
          "INSERT INTO usertable "
          "(firstname, lastname, email, uuid, password, nickname) "
          "VALUES ($1, $2, $3, $4, crypt($5, gen_salt('bf')), $6) RETURNING id",
          user_details.firstname,
          user_details.lastname,
          user_details.email,
          user_details.uuid.first ? &user_details.uuid.second : nullptr,
          user_details.password.first ? &user_details.password.second : nullptr,
          user_details.nickname);
      r["id"].to(id);
    }
    if (user_details.is_admin) {
      tx.exec_params(
          "INSERT INTO user_role (user_id, role_id) "
          "VALUES ($1, (SELECT id FROM role WHERE name='Admin')) "
          "ON CONFLICT (user_id, role_id) DO NOTHING",
          id);
    } else {
      tx.exec_params(
          "DELETE FROM user_role WHERE user_id=$1 "
          "AND role_id=(SELECT id FROM role WHERE name='Admin')",
          id);
    }
    tx.commit();
    return id;
  } catch (const std::exception &e) {
    std::cerr << "Error saving user details: "
              << e.what() << '\n';
    throw;
  }
}

void SessionPgDao::delete_users(const std::vector<long> &user_ids)
{
  try {
    work tx(*connection);
    const auto sql_ids = dao_helper::to_sql_array(user_ids);
    tx.exec_params("DELETE FROM usertable WHERE id=ANY($1)", sql_ids);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Error deleting users: "
              << e.what() << '\n';
    throw;
  }
}
