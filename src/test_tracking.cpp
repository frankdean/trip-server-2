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
#include "tracking_pg_dao.cpp"
#include "../trip-server-common/src/dao_helper.cpp"
#include "../trip-server-common/src/date_utils.cpp"
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <regex>

const std::string test_user_id = "test_user";

const std::map<std::string, std::string> valid_test_map =
{
  {"nickname", "fred"},
  {"from", "2022-08-08T00:00:00"},
  {"to", "2022-08-08T23:59:59"},
  {"max_hdop", "45"},
  {"offset", "20"},
  {"page_size", "10"},
  {"notes_only_flag", "on"},
  {"order", "DESC"}
};

const std::map<std::string, std::string> default_test_map =
{
  {"nickname", ""},
  {"from", ""},
  {"to", ""},
  {"max_hdop", ""},
  {"offset", ""},
  {"page_size", ""},
  {"notes_only_flag", ""},
  {"order", ""}
};

const std::map<std::string, std::string> invalid_test_map =
{
  {"nickname", ""},
  {"from", "rubbis"},
  {"to", "9999-99-08T99:99:99.000Z"},
  {"max_hdop", "abc"},
  {"offset", "def"},
  {"page_size", "ghi"},
  {"notes_only_flag", "jkl"},
  {"order", "mno"}
};

// Dates formatted as received in date-local format from the browser
const std::map<std::string, std::string> example_test_map =
{
  {"nickname", "colin"},
  {"from", "2022-08-10T00:00:00"},
  {"to", "2022-08-10T23:59:59"},
  {"max_hdop", "45"},
  {"offset", "20"},
  {"page_size", "10"},
  {"notes_only_flag", "on"},
  {"order", "DESC"}
};

bool test_valid_test_map()
{
  const TrackPgDao::location_search_query_params q(test_user_id, valid_test_map);
  std::ostringstream from;
  from << std::put_time(std::localtime(&q.date_from), "%FT%T%z");
  std::ostringstream to;
  to << std::put_time(std::localtime(&q.date_to), "%FT%T%z");
  std::cout << "From: \"" << from.str() << "\" to \"" << to.str() << "\"\n";
  const bool retval =
    q.user_id == test_user_id &&
    q.nickname == "fred" &&
    std::regex_match(from.str(), std::regex("2022-08-08T00:00:00(\\.000)?(\\+\\d{4})?")) &&
    std::regex_match(to.str(), std::regex("2022-08-08T23:59:59(\\.000)?(\\+\\d{4})?")) &&
    q.max_hdop == 45 &&
    q.page_offset == 20 &&
    q.page_size == 10 &&
    q.notes_only_flag &&
    q.order == dao_helper::descending;

  if (!retval)
    std::cout << "test_valid_test_map failed\n" << q << '\n';
  return retval;
}

bool test_default_test_map()
{
  const TrackPgDao::location_search_query_params q(test_user_id, default_test_map);
  const bool retval =
    q.user_id == test_user_id &&
    q.nickname == "" &&
    // The dates will default to now, so not much value in checking them
    // q.date_from.to_string() == "2022-08-07T23:00:00.000+0000" &&
    // q.date_to.to_string() == "2022-08-08T22:59:59.000+0000" &&
    q.max_hdop == -1 &&
    q.page_offset == 0 &&
    q.page_size == 10 &&
    !q.notes_only_flag &&
    q.order == dao_helper::ascending;

  if (!retval)
    std::cout << "test_default_test_map failed\n" << q << '\n';
  return retval;
}

bool test_invalid_test_map()
{
  const TrackPgDao::location_search_query_params q(test_user_id, invalid_test_map);
  const bool retval =
    q.user_id == test_user_id &&
    q.nickname == "" &&
    // The dates will default to now, so not much value in checking them
    // q.date_from.to_string() == "2022-08-07T23:00:00.000+0000" &&
    // q.date_to.to_string() == "2022-08-08T22:59:59.000+0000" &&
    q.max_hdop == -1 &&
    q.page_offset == 0 &&
    q.page_size == 10 &&
    !q.notes_only_flag &&
    q.order == dao_helper::ascending;

  if (!retval)
    std::cout << "test_invalid_test_map failed\n" << q << '\n';
  return retval;
}

bool test_example_test_map()
{
  const TrackPgDao::location_search_query_params q(test_user_id, example_test_map);
  std::ostringstream from;
  from << std::put_time(std::localtime(&q.date_from), "%FT%T");
  std::ostringstream to;
  to << std::put_time(std::localtime(&q.date_to), "%FT%T");
  const bool retval =
    q.user_id == test_user_id &&
    q.nickname == "colin" &&
    std::regex_match(from.str(), std::regex("2022-08-10T00:00:00(\\.000)?(\\+\\d{4})?")) &&
    std::regex_match(to.str(), std::regex("2022-08-10T23:59:59(\\.000)?(\\+\\d{4})?")) &&
    q.max_hdop == 45 &&
    q.page_offset == 20 &&
    q.page_size == 10 &&
    q.notes_only_flag &&
    q.order == dao_helper::descending;

  if (!retval)
    std::cout << "test_example_test_map failed\n" << q << '\n';
  return retval;
}

int main(void)
{
  return !(
      test_valid_test_map() &&
      test_default_test_map() &&
      test_invalid_test_map() &&
      test_example_test_map()
    );
}
