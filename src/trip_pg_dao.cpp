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
#include "trip_pg_dao.hpp"
#include "../trip-server-common/src/get_options.hpp"
#include "../trip-server-common/src/pg_pool.hpp"
#include <boost/locale.hpp>
#include <iostream>

using namespace pqxx;
using namespace boost::locale;
using namespace fdsd::utils;
using namespace fdsd::trip;

fdsd::utils::PgPoolManager* TripPgDao::pool_manager = nullptr;

TripPgDao::TripPgDao()
{
  try {
    connection = TripPgDao::pool_manager->get_connection();

    // if (GetOptions::verbose_flag) {
    //   std::cout
    //     << "Connected to database: " <<  connection->dbname() << '\n'
    //     << "Username: " << connection->username() << '\n'
    //     << "Hostname: "
    //     << (connection->hostname() ? connection->hostname() : "(null)")
    //     << '\n'
    //     << "Socket: " << connection->sock() << '\n'
    //     << "Backend PID: " << connection->backendpid() << '\n'
    //     << "Port: " << connection->port() << '\n'
    //     << "Backend version: " << connection->server_version() << '\n'
    //     << "Protocol version: " << connection->protocol_version() << "\n\n";
    // }

  } catch (const std::exception& e) {
    std::cerr << "Failure connecting to the database: "
              << e.what() << '\n';
    throw;
  }
}

TripPgDao::~TripPgDao()
{
  TripPgDao::pool_manager->free_connection(connection);
}

    // Error message displayed when a user attempts an operation they are not authorized for
TripPgDao::NotAuthorized::NotAuthorized()
  : BadRequestException(translate("User is not authorized for the requested operation"))
{
}

void TripPgDao::set_pool_manager(
    fdsd::utils::PgPoolManager* pool_manager)
{
  TripPgDao::pool_manager = pool_manager;
}

void TripPgDao::create_table(transaction_base &tx,
                             std::string table_name,
                             std::string create_table_sql,
                             bool overwrite)
{
  if (overwrite) {
    if (GetOptions::verbose_flag)
      std::cerr << "Dropping table " << table_name << '\n';
    tx.exec("DROP TABLE IF EXISTS " + table_name + " CASCADE");
  }
  tx.exec(create_table_sql);
}
