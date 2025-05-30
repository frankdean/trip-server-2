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
#include "../config.h"
#include "session_pg_dao.hpp"
#ifdef HAVE_TUI
#include "text-based_user_interface.hpp"
#endif // HAVE_TUI
#include "trip_application.hpp"
#include "trip_get_options.hpp"
#include "trip_pg_dao.hpp"
#include "trip_session_manager.hpp"
#include "../trip-server-common/src/pg_pool.hpp"
#include "../trip-server-common/src/session.hpp"
#include <boost/locale.hpp>
#include <cstdlib>
#include <chrono>
#include <csignal>
#include <memory>
#include <sstream>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <syslog.h>

using namespace boost::locale;
using namespace fdsd::utils;
using namespace fdsd::web;
using namespace fdsd::trip;

int main(int argc, char *argv[])
{
  const auto start = std::chrono::system_clock::now();
  openlog(PACKAGE_NAME, LOG_PERROR | LOG_PID, LOG_USER);
  generator gen;
  // Section 11.2.3 of GNU gettext states this is the default location for the
  // GNU library and also for packages adhering to its conventions.
  gen.add_messages_path("/usr/local/share/locale");
  gen.add_messages_domain("trip");
  std::locale::global(gen(""));
  std::cout.imbue(std::locale());
  std::clog.imbue(std::locale());
  // Logger logger("main", std::clog, Logger::info);
  try {
    TripGetOptions options;
    try {
      if (!options.init(argc, argv))
        return EXIT_SUCCESS;
    } catch (const GetOptions::UnexpectedArgumentException & e) {
      closelog();
      return EXIT_FAILURE;
    }

#ifdef HAVE_TUI
    if (options.tui_flag) {
      try {
        auto config =
          std::make_shared<TripConfig>(TripConfig(options.config_filename));
        // Running interactively only requires a single database connection
        auto pool_manager = std::make_shared<PgPoolManager>(
            config->get_db_connect_string(), 1);
        TripPgDao::set_pool_manager(pool_manager);
        TextUserInterface tui;
        const int exit_val = tui.run(argc, argv);
        closelog();
        return exit_val;
      } catch (const std::exception& e) {
        std::cerr << "Exception: " <<  e.what() << '\n';
        closelog();
        return EXIT_FAILURE;
      }
    }
#endif // HAVE_TUI

    const std::string test_table_name = "session";
    if (options.upgrade_flag) {
      try {
        auto config =
          std::make_shared<TripConfig>(TripConfig(options.config_filename));
        // Running the upgrade only requires a single database connection
        auto pool_manager = std::make_shared<PgPoolManager>(
            config->get_db_connect_string(), 1);
        TripPgDao::set_pool_manager(pool_manager);
        SessionPgDao session_dao;
        if (!session_dao.is_ready(test_table_name)) {
          // Message output when the database is not ready for use
          syslog(LOG_ERR, "%s",
                 (format(
                     translate("The database is not ready for use.  Cannot find the \"{1}\" table.")) % test_table_name
                   ).str().c_str());
          closelog();
          return EXIT_FAILURE;
        }
        // Message output when the database is being upgraded
        syslog(LOG_INFO, "%s", translate("Upgrading the database").str().c_str());
        session_dao.upgrade();
        closelog();
        return EXIT_SUCCESS;
      } catch (const std::exception& e) {
        std::cerr << "Exception: " <<  e.what() << '\n';
        closelog();
        return EXIT_FAILURE;
      }
    }

    TripApplication application(
        options.listen_address,
        options.port,
        options.config_filename);

    // std::string messages_path =
    //   application.get_config_value("messages_path",
    //                                "/usr/local/share/locale");
    // logger << Logger::info
    //        // Shows the file path being used to read translated messages
    //        << format(translate("Using '{1}' path for translated messages")
    //          ) % messages_path
    //        << Logger::endl;

#ifdef ENABLE_STATIC_FILES
    if (!options.doc_root.empty()) {
      if (options.doc_root.substr(options.doc_root.length() -1, 1) != "/")
        options.doc_root.append("/");
      application.set_root_directory(options.doc_root);
    }
#endif // ENABLE_STATIC_FILES
    // Initialize the global database pool and user session managers
    const std::string db_connect_str = application.get_db_connect_string();
    // std::cout << "Connecting to database with connect string: \""
    //           << db_connect_str << "\"\n";
    auto pool_manager = std::make_shared<PgPoolManager>(
        db_connect_str,
        application.get_pg_pool_size()
      );
    TripPgDao::set_pool_manager(pool_manager);
    TripSessionManager session_manager(application.get_config());
    TripSessionManager::set_session_manager(&session_manager);
    {
      SessionPgDao session_dao;
      if (!session_dao.is_ready(test_table_name)) {
        // Message output when the database is not ready for use
        syslog(LOG_ERR, "%s",
               (format(
                   translate("The database is not ready for use.  Cannot find the \"{1}\" table.")) % test_table_name
                 ).str().c_str());
        closelog();
        return EXIT_FAILURE;
      }
    }
    application.initialize_user_sessions(options.expire_sessions);

    application.initialize_workers(application.get_worker_count(),
                                   pool_manager);
    std::stringstream msg01;
    msg01
      // Label for the version text of the application
      << PACKAGE << ' ' << translate("version") << ' ' << VERSION
      // Label for the web address the application is listening at
      << ' ' << translate("listening at") << " http://"
      << options.listen_address << ':' << options.port
      << application.get_application_prefix_url();
    syslog(LOG_INFO, "%s", msg01.str().c_str());
#ifdef ENABLE_STATIC_FILES
    // Output when the application has been built to allow serving static files.
    syslog(LOG_INFO, "%s", translate("The application has been built to serve static files.").str().c_str());
    std::stringstream msg02;
    msg02 << format(
        // Displays the name of the directory that static files will be served from.
        translate("Static files will be served from the {1} directory.")) %
      options.doc_root;
    syslog(LOG_INFO, "%s", msg02.str().c_str());
#ifdef ENABLE_DIRECTORY_LISTING
    std::stringstream msg03;
    msg03 <<
      // Output when the application has been built to allow listing the
      // contents of directories.
      "Additionally, listing directory contents has also been enabled.";
    syslog(LOG_INFO, "%s", msg03.str().c_str());
#endif

#endif
    const auto finish = std::chrono::system_clock::now();
    const std::chrono::duration<double, std::milli> diff = finish - start;
    std::stringstream msg04;
    msg04
      // Message showing how long the application took to startup in milliseconds
      << format(translate("Started application in {1} ms"))
      % diff.count();
    syslog(LOG_INFO, "%s", msg04.str().c_str());
    auto config = application.get_config();
    application.run();
    // Text displayed when the application ends
    syslog(LOG_INFO, "%s", translate("Bye!").str().c_str());
  } catch (const std::exception& e) {
    syslog(LOG_ERR, "Application exiting with error: %s", e.what());
    closelog();
    return EXIT_FAILURE;
  }
  // Any Boost translation calls are out of scope here onwards
  closelog();
  return EXIT_SUCCESS;
}
