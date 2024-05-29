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
#include "trip_get_options.hpp"
#include <iostream>
#include <locale>
#include <boost/locale.hpp>

using namespace fdsd::trip;
using namespace boost::locale;

int TripGetOptions::expire_sessions = 0;
int TripGetOptions::upgrade_flag = 0;
int TripGetOptions::tui_flag = 0;

TripGetOptions::TripGetOptions() : GetOptions()
{
#ifdef ALLOW_STATIC_FILES
  doc_root=DATADIR "/" PACKAGE "/resources";
#endif
}

#ifdef HAVE_GETOPT_H

const char TripGetOptions::short_opts[] = "hs:p:c:vuVe"
#ifdef HAVE_TUI
  "i"
#endif
#ifdef ALLOW_STATIC_FILES
  "r:"
#endif
  ;

struct option TripGetOptions::long_options[] = {
  // Name                 Argument           Flag              Shortname
  {"help",                no_argument,       NULL,             'h'},
  {"listen",              required_argument, NULL,             's'},
  {"port",                required_argument, NULL,             'p'},
#ifdef ALLOW_STATIC_FILES
  {"root",                required_argument, NULL,             'r'},
#endif
  {"config-file",         required_argument, NULL,             'c'},
  {"verbose",             no_argument,       &verbose_flag,    1},
  {"version",             no_argument,       NULL,             'v'},
  {"expire_sessions",     no_argument,       &expire_sessions, 1},
  {"upgrade",             no_argument,       &upgrade_flag,    1},
#ifdef HAVE_TUI
  {"interactive",         no_argument,       &tui_flag,    1},
#endif // HAVE_TUI
  {NULL, 0, NULL, 0}
};

/**
 * \return true if the application should continue, false if the application
 * should exit.
 */
bool TripGetOptions::handle_option(int c)
{
  // std::cout << "TripGetOptions::handle option: " << (char) c << '\n';
  switch (c) {
    case 'e':
      expire_sessions = 1;
      break;
    case 'u':
      upgrade_flag = 1;
      break;
#ifdef HAVE_TUI
    case 'i':
      tui_flag = 1;
      break;
#endif // HAVE_TUI
    default:
      return GetOptions::handle_option(c);
  } // switch
  return true;
}
#endif // HAVE_GETOPT_H

void TripGetOptions::show_version_info() const
{
  std::cout
    << PACKAGE << " " << VERSION << '\n'
    << "Copyright (C) 2022-2024 Frank Dean\n"
    << "This program comes with ABSOLUTELY NO WARRANTY.\n"
    << "This is free software, and you are welcome to redistribute it\n"
    << "under certain conditions.\n"
    << "See https://www.gnu.org/licenses/gpl-3.0.html for details\n";
}

void TripGetOptions::usage(std::ostream& os) const
{
#ifdef HAVE_GETOPT_H
  os
    // Heading before showing command line usage and options with the program name as the parameter
    << format(translate("Usage:\n {1} [OPTIONS]\n\nOptions:\n")) % program_name
    // Command line short description for the --help option
    << "  -h, --help\t\t\t\t" << translate("show this help, then exit") << '\n'
    // Command line short description for the --version option
    << "  -v, --version\t\t\t\t" << translate("show version information, then exit") << '\n'
    // Command line short description for the --listen option
    << "  -s, --listen=ADDRESS\t\t\t" << translate("listen address, e.g. 0.0.0.0") << '\n'
    // Command line short description for the --port option
    << "  -p, --port=PORT\t\t\t" << translate("port number, e.g. 8080") << '\n'
#ifdef ALLOW_STATIC_FILES
    // Command line short description for the --root option
    << "  -r, --root=DIRECTORY\t\t\t" << translate("document root directory") << '\n'
#endif
    // Command line short description for the --expire-sessions option
    << "  -e, --expire-sessions\t\t\t" << translate("expires any active user web sessions") << '\n'
    // Command line short description for the --config-file option
    << "  -c, --config-file=FILENAME\t\t" << translate("configuration file name") << '\n'
    // Command line short description for the --upgrade option
    << "  -u, --upgrade\t\t\t\t" << translate("upgrade database") << '\n'
#ifdef HAVE_TUI
    // Command line short description for the --interactive option
    << "  -i  --interactive\t\t\t" << translate("run an interactive session") << '\n'
#endif // HAVE_TUI
    // Command line short description for the --verbose option
    << "  -V, --verbose\t\t\t\t" << translate("verbose output") << '\n';
#else
#ifdef ALLOW_STATIC_FILES
  // Example usage when the application is configured to serve static files
  os << format(translate("Usage: {1} <address> <port> <doc_root>")) % program_name;
#else
  // Example usage when the application is not configured to serve static files
  os << format(translate("Usage: {1} <address> <port>")) % program_name;
#endif
  os
    << '\n'
    // Heading before showing command line usage
    << translate("Example:\n")
    << "    http-server-sync 0.0.0.0 8080"
#ifdef ALLOW_STATIC_FILES
    << " ."
#endif
    << '\n';
#endif // HAVE_GETOPT_H
}

#ifdef HAVE_GETOPT_H
const char* TripGetOptions::get_short_options() const
{
  return short_opts;
}

const struct option* TripGetOptions::get_long_options() const
{
  return long_options;
}
#endif // HAVE_GETOPT_H
