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
#include "../trip-server-common/src/date_utils.hpp"
#include <iostream>

using namespace pqxx;
using namespace fdsd::utils;
using namespace fdsd::web;
using namespace fdsd::trip;

const std::string SessionPgDao::tracks_query_key = "tracks";

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
                   session.get_last_updated_time_t());
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
                     session.second.get_last_updated_time_t());
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
      "FROM user_role ur JOIN role r ON ur.user_id=id "
      "WHERE r.name='Admin' AND ur.user_id='" + tx.esc(user_id) + '\'');
  tx.commit();
  return !r.empty() && r[0][0].as<int>() > 0;
}

void SessionPgDao::upgrade()
{
  work tx(*connection);
  try {
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

}
