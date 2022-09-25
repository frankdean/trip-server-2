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
#ifndef SESSION_PG_DAO_HPP
#define SESSION_PG_DAO_HPP

#include "trip_pg_dao.hpp"
#include "../trip-server-common/src/session.hpp"
#include <string>

namespace fdsd {
namespace trip {

class SessionPgDao : public TripPgDao {
  static const std::string insert_session_ps_name;
  static const std::string create_session_table_sql;
  static const std::string create_session_data_table_sql;
  static const std::string insert_session_sql;
public:
  static const std::string tracks_query_key;
  void save_value(std::string session_id, std::string key, std::string value);
  std::string get_value(std::string session_id, std::string key);
  void create_session_table(bool overwrite = false);
  void save_session(std::string session_id,
                    const fdsd::web::Session session);
  void save_sessions(const fdsd::web::session_map sessions);
  void delete_all_sessions();
  void load_sessions(fdsd::web::session_map &sessions);
  void invalidate_session(const std::string session_id);
  bool validate_password(std::string email, std::string password);
  std::string get_user_id_by_email(const std::string email);
  bool is_admin(std::string user_id);
  void upgrade();
};

} // namespace trip
} // namespace fdsd

#endif // SESSION_PG_DAO_HPP
