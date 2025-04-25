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
#include "tracking_pg_dao.cpp"
#include "../trip-server-common/src/dao_helper.cpp"
#include "../trip-server-common/src/date_utils.cpp"
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <regex>

const std::string test_user_id = "test_user";

//////////////////////////////////////////////////////////////////////////////
// Tests for TrackPgDao::location_search_query_params
//////////////////////////////////////////////////////////////////////////////

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
  // std::cout << "From: \"" << from.str() << "\" to \"" << to.str() << "\"\n";
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
    std::cerr << "test_valid_test_map failed\n" << q << '\n';
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
    std::cerr << "test_default_test_map failed\n" << q << '\n';
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
    std::cerr << "test_invalid_test_map failed\n" << q << '\n';
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
    std::cerr << "test_example_test_map failed\n" << q << '\n';
  return retval;
}

const std::map<std::string, std::string> tracked_loc_qp_valid_map_ISO8601 =
{
  {"lng", "2.294521"},
  {"lat", "48.858222"},
  {"altitude", "73.000001"},
  {"time", "2022-10-13T12:50:46.321+01:00"},
  {"hdop", "15.8"},
  {"speed", "3.2"},
  {"bearing", "359.56789"},
  {"sat", "30"},
  {"prov", "test"},
  {"batt", "98.7"},
  {"note", "Test note"}
};

const std::map<std::string, std::string> tracked_loc_qp_without_time_and_lon =
{
  {"lon", "2.294521"},
  {"lat", "48.858222"},
  {"altitude", "73.000001"},
  {"hdop", "15.8"},
  {"speed", "3.2"},
  {"bearing", "359.56789"},
  {"sat", "30"},
  {"prov", "test"},
  {"batt", "98.7"},
  {"note", "Test note"}
};

bool test_tracked_location_query_params_constructor_01()
{
  TrackPgDao::tracked_location_query_params q(test_user_id, tracked_loc_qp_valid_map_ISO8601);
  const std::string s = tracked_loc_qp_valid_map_ISO8601.at("time");
  DateTime dt(s);
  const bool retval =
    q.user_id == test_user_id &&
    q.time_point == dt.time_tp() &&
    std::round((q.longitude - 2.294521) * 1e6) == 0 &&
    std::round((q.latitude - 48.858222) * 1e6) == 0 &&
    q.altitude.has_value() && std::round((q.altitude.value() - 73.000001) * 1e6) == 0 &&
    q.hdop.has_value() && std::round((q.hdop.value() - 15.8) * 10) == 0 &&
    q.speed.has_value() && std::round((q.speed.value() - 3.2) * 10) == 0 &&
    q.bearing.has_value() && std::round((q.bearing.value() - 359.56789) * 1e6) == 0 &&
    q.satellite_count.has_value() && q.satellite_count.value() == 30 &&
    q.provider.has_value() && q.provider.value() == "test" &&
    q.battery.has_value() && std::round((q.battery.value() - 98.7) * 10) == 0 &&
    q.note.has_value() && q.note.value() == "Test note";
  if (!retval) {
    std::cerr
      << "test_tracked_location_query_params_constructor_01() failed: "
      << q << '\n';
    if (q.user_id != test_user_id)
      std::cerr << "user_id\n";
    if (q.time_point != dt.time_tp())
      std::cerr << "time_point\n";
    if (std::round((q.longitude - 2.294521) * 1e6) != 0)
      std::cerr << "longitude\n";
    if (std::round((q.latitude - 48.858222) * 1e6) != 0)
      std::cerr << "latitude\n";
    if (q.altitude.has_value() && std::round((q.altitude.value() - 73.000001) * 1e6) != 0)
      std::cerr << "altitude.has_value() && altitude.value()\n";
    if (q.hdop.has_value() && std::round((q.hdop.value() - 15.8) * 10) != 0)
      std::cerr << "hdop.has_value() && hdop.value()\n";
    if (q.speed.has_value() && std::round((q.speed.value() - 3.2) * 10) != 0)
      std::cerr << "speed.has_value() && speed.value()\n";
    if (q.bearing.has_value() && std::round((q.bearing.value() - 359.56789) * 1e6) != 0)
      std::cerr << "bearing.has_value() && bearing.value()\n";
    if (q.satellite_count.has_value() && q.satellite_count.value() != 30)
      std::cerr << "satellite_count.has_value() && q.satellitcount.value()\n";
    if (!q.provider.has_value() || q.provider.value() != "test")
      std::cerr << "provider\n";
    if (q.battery.has_value() && std::round((q.battery.value() - 98.7) * 10) != 0)
      std::cerr << "battery.has_value() && battery.value(): "
                << std::fixed << std::setprecision(19)
                << q.battery.value() << '\n';
    if (!q.note.has_value() || q.note.value() != "Test note")
      std::cerr << "note\n";
  }
  return retval;
}

// test no time parameter - should use the current time.
// Also test using `lon` instead of `lng` for longitude.
bool test_tracked_location_query_params_constructor_02()
{
  auto test_map = tracked_loc_qp_without_time_and_lon;
  TrackPgDao::tracked_location_query_params q(test_user_id, test_map);
  // Where no time is specified, we expect the current time to be used.
  const DateTime now;
  const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      q.time_point - now.time_tp());
  // std::cout << "Diff: " << std::fixed << std::setprecision(6) << diff.count() << '\n';
  // Allow a generous tolerance between the call and the current time being
  // captured.
  const bool retval =
    std::abs(diff.count()) < 2000 &&
    std::round((q.longitude - 2.294521) * 1e6) == 0;
  if (!retval) {
    std::cerr
      << "test_tracked_location_query_params_constructor_01() failed: "
      << q << '\n';
    if (std::abs(diff.count()) != 0)
      std::cerr << "Expected time \"" << now.get_time_as_iso8601_gmt()
                << "\" differs by " << diff.count() << "ms\n";
    if (std::round((q.longitude - 2.294521) * 1e6) != 0)
      std::cerr << "longitude\n";
  }
  return retval;
}

// test mstime parameter
bool test_tracked_location_query_params_constructor_03()
{
  auto test_map = tracked_loc_qp_without_time_and_lon;
  DateTime test_date("2022-10-13T17:25:30.179+0100");
  test_map["mstime"] = std::to_string(test_date.get_ms());
  TrackPgDao::tracked_location_query_params q(test_user_id, test_map);
  const bool retval = (q.time_point == test_date.time_tp());
  if (!retval)
    std::cerr
      << "test_tracked_location_query_params_constructor_03() failed\n";
  return retval;
}

// test unixtime parameter
bool test_tracked_location_query_params_constructor_04()
{
  auto test_map = tracked_loc_qp_without_time_and_lon;
  const DateTime test_date("2022-10-13T17:25:30+0100");
  test_map["unixtime"] = std::to_string(std::llround(test_date.get_ms() / 1000));
  const TrackPgDao::tracked_location_query_params q(test_user_id, test_map);
  const bool retval = (q.time_point == test_date.time_tp());
  if (!retval)
    std::cerr
      << "test_tracked_location_query_params_constructor_04() failed"
      << q << '\n';
  return retval;
}

// test timestamp parameter
bool test_tracked_location_query_params_constructor_05()
{
  auto test_map = tracked_loc_qp_without_time_and_lon;
  const DateTime test_date("2022-10-13T17:25:30+0100");
  test_map["timestamp"] = std::to_string(std::llround(test_date.get_ms() / 1000));
  const TrackPgDao::tracked_location_query_params q(test_user_id, test_map);
  const bool retval = (q.time_point == test_date.time_tp());
  if (!retval)
    std::cerr
      << "test_tracked_location_query_params_constructor_05() failed"
      << q << '\n';
  return retval;
}

int main(void)
{
  try {
    return !(
        test_valid_test_map()
        && test_default_test_map()
        && test_invalid_test_map()
        && test_example_test_map()
        && test_tracked_location_query_params_constructor_01()
        && test_tracked_location_query_params_constructor_02()
        && test_tracked_location_query_params_constructor_03()
        && test_tracked_location_query_params_constructor_04()
        && test_tracked_location_query_params_constructor_05()
      );
  } catch (const std::exception &e) {
    std::cerr << "Tests failed with: " << e.what() << '\n';
    return 1;
  }
}
