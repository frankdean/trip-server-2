// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2025 Frank Dean <frank.dean@fdsd.co.uk>

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU Affero General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
    for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef SESSION_PG_DAO_HPP
#define SESSION_PG_DAO_HPP

#include "trip_pg_dao.hpp"
#include "../trip-server-common/src/session.hpp"
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace fdsd {
namespace trip {

class SessionPgDao : public TripPgDao {
public:
  enum search_type {
    partial,
    exact
  };
private:
  static const std::string insert_session_ps_name;
  static const std::string create_session_table_sql;
  static const std::string create_session_data_table_sql;
  static const std::string insert_session_sql;
  void append_user_search_where_clause(
      pqxx::work &tx,
      std::string email,
      std::string nickname,
      search_type search_type,
      std::ostream &sql);
protected:
  bool is_admin(pqxx::work &tx, std::string user_id);
public:
  struct user {
    std::optional<long> id;
    std::string firstname;
    std::string lastname;
    std::string email;
    std::optional<std::string> uuid;
    std::optional<std::string> password;
    std::string nickname;
    bool is_admin;
    user() : id(),
             firstname(),
             lastname(),
             email(),
             uuid(),
             password(),
             nickname(),
             is_admin(false) {}
  };
  // enum user_role {
  //   admin,
  //   user
  // };
  struct tile_usage_metric {
    int month;
    int year;
    long cumulative_total;
    int quantity;
    tile_usage_metric() : month(), year(), cumulative_total(), quantity() {}
  };
  struct tile_report {
    std::optional<long> total;
    std::optional<std::chrono::system_clock::time_point> time;
    std::vector<tile_usage_metric> metrics;
    tile_report() : total(), time(), metrics() {}
  };
  static const std::string coordinate_format_key;
  static const std::string tracks_query_key;
  static const std::string location_history_key;
  static const std::string itinerary_features_key;
  static const std::string itinerary_page_key;
  static const std::string itinerary_search_page_key;
  void save_value(std::string session_id, std::string key, std::string value);
  std::string get_value(std::string session_id, std::string key);
  void remove_value(std::string session_id, std::string key);
  void remove_values(std::string session_id, const std::vector<std::string> &keys);
  void clear_copy_buffers(std::string session_id);
  void create_session_table();
  void save_session(std::string session_id,
                    const fdsd::web::Session session);
  void save_sessions(const fdsd::web::session_map sessions);
  void delete_all_sessions();
  void load_sessions(fdsd::web::session_map &sessions);
  void invalidate_session(const std::string session_id);
  bool validate_password(std::string email, std::string password);
  bool validate_password_by_user_id(std::string user_id, std::string password);
  void change_password(std::string user_id, std::string new_password);
  std::string get_user_id_by_email(const std::string email);
  tile_report get_tile_usage_metrics(int month_count);
  bool is_admin(std::string user_id);
  SessionPgDao::user get_user_details_by_user_id(std::string user_id);
  long save(const SessionPgDao::user &user_details);
  void delete_users(const std::vector<long> &user_ids);
  long get_search_users_by_nickname_count(
      std::string email,
      std::string nickname,
      SessionPgDao::search_type search_type);
  std::vector<user> search_users_by_nickname(
      std::string email,
      std::string nickname,
      SessionPgDao::search_type search_type,
      std::uint32_t offset,
      int limit);
  /**
   * \return current value of default_text_search_config, stripped of pg_catalog
   * prefix, e.g. 'english'.  Defaults to 'english' it not set.
   */
  std::string get_default_text_search_config();
  void upgrade();
};

} // namespace trip
} // namespace fdsd

#endif // SESSION_PG_DAO_HPP
