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
#ifndef TRIP_PG_DAO_HPP
#define TRIP_PG_DAO_HPP

#include "../trip-server-common/src/pg_pool.hpp"
#include "../trip-server-common/src/http_request_handler.hpp"
#include <exception>
#include <memory>
#include <string>
#include <pqxx/pqxx>

namespace fdsd {
namespace trip {

class TripPgDao {
protected:
#ifdef HAVE_LIBPQXX7
  std::shared_ptr<pqxx::connection> connection;
#else
  std::shared_ptr<pqxx::lazyconnection> connection;
#endif
  static std::shared_ptr<fdsd::utils::PgPoolManager> pool_manager;
  void create_table(pqxx::transaction_base &tx, std::string table_name,
                    std::string create_table_sql, bool overwrite);
public:
  TripPgDao();
  virtual ~TripPgDao();

  /// Exception thrown when a user attempts to perform a forbidden action,
  /// e.g. access an itinerary they do not own.
  class TripForbiddenException : public web::ForbiddenException {
  public:
    TripForbiddenException();
  };

  static void set_pool_manager
      (std::shared_ptr<fdsd::utils::PgPoolManager> pool_manager);
  bool is_ready(std::string test_table_name,
                int retry_interval_secs = 10,
                int max_retries = 12);
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_PG_DAO_HPP
