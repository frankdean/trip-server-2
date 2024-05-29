// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2024 Frank Dean <frank.dean@fdsd.co.uk>

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
#include <syslog.h>
#include <thread>

using namespace pqxx;
using namespace boost::locale;
using namespace fdsd::utils;
using namespace fdsd::trip;

std::shared_ptr<fdsd::utils::PgPoolManager> TripPgDao::pool_manager = nullptr;

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
    syslog(LOG_ERR,
           "Failure connecting to the database: %s",
           e.what());
    throw;
  }
}

TripPgDao::~TripPgDao()
{
  TripPgDao::pool_manager->free_connection(connection);
}

// Error message displayed when a user attempts a forbidden operation
TripPgDao::TripForbiddenException::TripForbiddenException()
  : ForbiddenException(translate("User is forbidden to perform the requested operation"))
{
}

void TripPgDao::set_pool_manager(
    std::shared_ptr<fdsd::utils::PgPoolManager> pool_manager)
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

bool TripPgDao::is_ready(std::string test_table_name,
                         int retry_interval_secs,
                         int max_retries)
{
  int retry_count = max_retries;
  while (retry_count-- > 0) {
    try {
      work tx(*connection);
      auto row = tx.exec_params1(
          "SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name=$1 AND table_schema='public' AND table_type='BASE TABLE')",
          test_table_name);
      return row[0].as<bool>();
    } catch (const failure& e) {
      syslog(LOG_DEBUG,
             "Error checking whether database is ready: %s",
             e.what());
      std::this_thread::sleep_for(std::chrono::seconds(retry_interval_secs));
    } catch (const std::exception& e) {
      syslog(LOG_DEBUG,
             "Exception connecting to the database: %s",
             e.what());
      std::this_thread::sleep_for(std::chrono::seconds(retry_interval_secs));
    }
  }
  return false;
}
