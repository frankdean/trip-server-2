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
#ifndef TRIP_PG_DAO_HPP
#define TRIP_PG_DAO_HPP

#include "../trip-server-common/src/pg_pool.hpp"
#include <exception>
#include <string>
#include <pqxx/pqxx>

namespace fdsd {
namespace trip {

class TripPgDao {
protected:
  std::shared_ptr<pqxx::lazyconnection> connection;
  static fdsd::utils::PgPoolManager* pool_manager;
  void create_table(pqxx::transaction_base &tx, std::string table_name,
                    std::string create_table_sql, bool overwrite);
public:
  TripPgDao();
  virtual ~TripPgDao();

  /// Exception thrown when a user attempts to perform an unauthorised action,
  /// e.g. access an itinerary they do not own.
  class NotAuthorized : public std::exception {
  public:
    virtual const char* what() const throw() override {
      return "User not authorized to perform the requested action";
    }
  };

  static void set_pool_manager(fdsd::utils::PgPoolManager* pool_manager);
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_PG_DAO_HPP
