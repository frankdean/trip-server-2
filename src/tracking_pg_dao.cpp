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
#include "tracking_pg_dao.hpp"
#ifdef HAVE_GDAL
#include "elevation_tile.hpp"
#endif
#include "../trip-server-common/src/date_utils.hpp"
#include <iomanip>
#include <locale>
#include <sstream>
// #ifdef HAVE_CXX17
// #include <optional>
// #endif

using namespace fdsd::trip;
using namespace fdsd::utils;
using namespace pqxx;
using json = nlohmann::json;

std::mutex TrackPgDao::location_search_query_params::mutex;

namespace fdsd {
namespace trip {

void to_json(json& j, const TrackPgDao::location_search_query_params& qp)
{
  TrackPgDao::location_search_query_params::to_json(j, qp);
}

void from_json(const json& j, TrackPgDao::location_search_query_params& qp)
{
  TrackPgDao::location_search_query_params::from_json(j, qp);
}

} // namespace trip
} // namespace fdsd

std::string TrackPgDao::tracked_location::to_string() const
{
  DateTime dt(time_point);
  std::ostringstream os;
  os <<
    location::to_string() << ", "
    "time_point: \"" << dt.get_time_as_iso8601_gmt() << "\", " <<
    std::fixed << std::setprecision(1);
  if (hdop.has_value())
    os << "hdop: " << hdop.value() << ", ";
  if (speed.has_value())
    os << "speed: " << speed.value() << ", ";
  os << std::setprecision(5);
  if (bearing.has_value())
    os << "bearing: " << bearing.value() << ", ";
  os << std::setprecision(0);
  if (satellite_count.has_value())
    os << "satellite_count: " << satellite_count.value() << ", ";
  os << "provider: " << (provider.has_value() ? '"' + provider.value() + '"' : "null") << ", " << std::setprecision(1);
  if (battery.has_value())
    os <<"battery: " << battery.value() << ", ";
  os << "note: \"" << (note.has_value() ? '"' + note.value() + '"' : "null") << "\"";
  return os.str();
}

std::string TrackPgDao::tracked_location_query_params::to_string() const
{
  std::ostringstream os;
  os
    << "user_id: \"" << user_id << "\", "
    << tracked_location::to_string();
  return os.str();
}

TrackPgDao::location_search_query_params::location_search_query_params()
{
  std::lock_guard<std::mutex> lock(mutex);
  std::time_t now = std::time(nullptr);
  std::tm* start = std::localtime(&now);
  start->tm_hour = 0;
  start->tm_min = 0;
  start->tm_sec = 0;
  date_from = std::mktime(start);
  std::tm* end = std::localtime(&now);
  end->tm_hour = 23;
  end->tm_min = 59;
  end->tm_sec = 59;
  date_to = std::mktime(end);
  // std::cout << "location_search_query_params() From: " << std::put_time(std::localtime(&date_from), "%FT%T%z") << '\n';
  // std::cout << "location_search_query_params() To: " << std::put_time(std::localtime(&date_to), "%FT%T%z") << '\n';
}

TrackPgDao::location_search_query_params::location_search_query_params(
    std::string user_id, const std::map<std::string, std::string> &params) : user_id(user_id)
{
  nickname = get_value(params, "nickname");
  date_from = get_date(params, "from");
  date_to = get_date(params, "to");
  max_hdop = get_int(params, "max_hdop");
  notes_only_flag = (get_value(params, "notes_only_flag") == "on");
  order = get_result_order(params, "order");
  page = get_long(params, "page", 0);
  page_offset = get_long(params, "offset", 0);
  page_size = get_long(params, "page_size", 10);
  // std::cout << "location_search_query_params() From: " << std::put_time(std::localtime(&date_from), "%FT%T%z") << '\n';
  // std::cout << "location_search_query_params() To: " << std::put_time(std::localtime(&date_to), "%FT%T%z") << '\n';
}

void TrackPgDao::location_search_query_params::to_json(
    json& j, const location_search_query_params& qp)
{

  j = json{
    {"user_id", qp.user_id},
    {"nickname", qp.nickname},
    {"date_from", qp.date_from},
    {"date_to", qp.date_to},
    {"max_hdop", qp.max_hdop},
    {"notes_only_flag", qp.notes_only_flag},
    {"order", (qp.order == ascending ? "ASC" : "DESC")},
    {"page", qp.page},
    {"page_offset", qp.page_offset},
    {"page_size", qp.page_size}
  };
}

void TrackPgDao::location_search_query_params::from_json(
    const json& j, location_search_query_params& qp)
{
  j.at("user_id").get_to(qp.user_id);
  j.at("nickname").get_to(qp.nickname);
  j.at("date_from").get_to(qp.date_from);
  j.at("date_to").get_to(qp.date_to);
  j.at("max_hdop").get_to(qp.max_hdop);
  j.at("notes_only_flag").get_to(qp.notes_only_flag);
  std::string order;
  j.at("order").get_to(order);
  qp.order = (order == "ASC" ? ascending : descending);
  j.at("page").get_to(qp.page);
  j.at("page_offset").get_to(qp.page_offset);
  j.at("page_size").get_to(qp.page_size);
}

std::string TrackPgDao::location_search_query_params::to_string() const
{
  std::ostringstream os;
  os <<
    "user_id: \"" << user_id << "\", "
    "nickname: \"" << nickname << "\", "
    "date_from: \"" << std::put_time(std::localtime(&date_from), "%FT%T%z") << "\", "
    "date_to: \"" << std::put_time(std::localtime(&date_to), "%FT%T%z") << "\", "
    "max_hdop: \"" << max_hdop << "\", "
    "notes_only_flag: \"" << notes_only_flag << "\", "
    "order: \"" << (order == ascending ? "ASC" : "DESC") << "\", "
    "page: \"" << page << "\", "
    "page_offset: \"" << page_offset << "\", "
    "page_size: \"" << page_size << "\", ";
  return os.str();
}

std::map<std::string, std::string> TrackPgDao::location_search_query_params::query_params()
{
  std::ostringstream from;
  std::ostringstream to;
  from << std::put_time(std::localtime(&date_from), "%FT%T%z");
  to <<  std::put_time(std::localtime(&date_to), "%FT%T%z");
  std::map<std::string, std::string> m;
  // m["user_id"] = user_id;
  m["nickname"] = nickname;
  m["from"] = from.str();
  m["to"] = to.str();
  m["max_hdop"] = std::to_string(max_hdop);
  m["notes_only_flag"] = notes_only_flag ? "on" : "off";
  m["order"] = (order == ascending ? "ASC" : "DESC");
  m["page"] = std::to_string(page);
  return m;
}

/**
 * Constructor for tracked location query parameters.
 *
 * There are various ways for the query to express the time the location refers
 * to, listed in the priority order in which they are evaluated, highest
 * priority first:
 *
 *  mstime - unix time milliseconds
 *
 *  unixtime - unix time seconds
 *
 *  timestamp - synonym for unixtime
 *
 *  time - ISO 8601 format time
 *
 *  offset - Offset to apply to the time value being passed in
 *  seconds. e.g. 3600 to add one hour. This is a workaround to situations
 *  where it is known that the time value is consistently incorrectly
 *  reported. e.g. A bug causing the GPS time to be one hour slow. Can be a
 *  comma separated list of the same length as the offsetprovs parameter.
 *
 *  offsetprovs - sed in conjunction with the prov parameter to apply offsets
 *  per location provider. E.g. setting offset to '3600' and offsetprovs to
 *  'gps' will only apply the offset to locations submitted with the prov
 *  parameter matching 'gps'. To apply offset to more than one provider, use
 *  comma separated lists of the same length. E.g. set offset to '3600,7200'
 *  and offsetprovs to 'gps,network' to add 1 hour to gps times and 2 hours
 *  to network times.
 *
 *  msoffset - Same as the offset parameter above, but in milliseconds
 *  e.g. 1000 to add one second.
 *
 *  lat - decimal degrees
 *
 *  lng/lon - decimal degrees
 *
 *  altitude
 *
 *  hdop
 *
 *  speed
 *
 *  bearing - Bearing in decimal degrees
 *
 *  sat
 *
 *  prov
 *
 *  batt
 *
 *  note
 *
 * \throws std::invalid_argument if any of the parameters hold invalid values.
 *
 * \throws std::out_of_range if any of the parameters contain invalid values.
 *
 */
TrackPgDao::tracked_location_query_params::tracked_location_query_params(
    std::string user_id,
    const std::map<std::string,
    std::string> &params) : tracked_location()
{
  this->user_id = user_id;
  id = std::optional<long>();
  std::string s = get_value(params, "lat");
  if (s.empty())
    s = get_value(params, "latitude");
  latitude = std::stod(s);
  if (latitude < -90 || latitude > 90)
    throw std::invalid_argument("Invalid latitude value");
  s = get_value(params, "lng");
  if (s.empty())
    s = get_value(params, "lon");
  if (s.empty())
    s = get_value(params, "long");
  if (s.empty())
    s = get_value(params, "longitude");
  longitude = std::stod(s);
  if (longitude < -180 || longitude > 180)
    throw std::invalid_argument("Invalid longitude value");
  altitude = get_optional_double_value(params, "altitude");
  // Default to the current time
  time_point = std::chrono::system_clock::now();
  // Convert the possible time parameters
  s = get_value(params, "mstime");
  if (!s.empty()) {
    // std::cout << "Using mstime parameter\n";
    const long long ms = std::stoll(s);
    time_point = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(ms));
  } else {
    s = get_value(params, "unixtime");
    if (s.empty())
      s = get_value(params, "timestamp");
    if (!s.empty()) {
      try {
        const long long seconds = std::stoll(s);
        time_point = std::chrono::system_clock::time_point(
            std::chrono::seconds(seconds));
      } catch (const std::invalid_argument &e) {
        // Try as time string
        DateTime dt(s);
        time_point = dt.time_tp();
      }
    } else {
      s = get_value(params, "time");
      if (!s.empty()) {
        // std::cout << "Using time parameter\n";
        DateTime dt(s);
        time_point = dt.time_tp();
      }
    }
  }
  hdop = get_optional_float_value(params, "hdop");
  speed = get_optional_float_value(params, "speed");
  bearing = get_optional_double_value(params, "bearing");
  satellite_count = get_optional_int_value(params, "sat");
  provider = get_optional_value(params, "prov");
  battery = get_optional_float_value(params, "batt");
  note = get_optional_value(params, "note");
}

std::string TrackPgDao::nickname_result::to_string() const
{
  std::ostringstream os;
  os << "nickname: \"" << nickname << "\", "
    "nicknames: {";
  bool first = true;
  for (const std::string& nickname : nicknames) {
    if (!first)
      os << ", ";
    os << '"' << nickname << '"';
    first = false;
  }
  os << '}';
  return os.str();
}

void TrackPgDao::append_location_query_where_clause(
    std::ostringstream& os,
    const pqxx::work& tx,
    const location_search_query_params& qp) const
{
  os
    << "WHERE user_id = '" << tx.esc(qp.user_id) << "' AND "
    "time >= '" << std::put_time(std::localtime(&qp.date_from), "%FT%T")
    << "' AND time <= '"
    << std::put_time(std::localtime(&qp.date_to), "%FT%T") << '\'';

  if (qp.max_hdop >= 0)
    os << " AND hdop <= " << qp.max_hdop;

  if (qp.notes_only_flag)
    os << " AND note IS NOT NULL";

}

TrackPgDao::tracked_locations_result
    TrackPgDao::get_tracked_locations_for_user(
        const location_search_query_params& qp,
        int maximum) const
{
  // std::cout << "TrackPgDao::get_tracked_locations_for_user() - "
  //           << "search criteria: " << qp << '\n';
  tracked_locations_result retval;
  retval.date_from = qp.date_from;
  retval.date_to = qp.date_to;
  work tx(*connection);
  std::ostringstream ss;
  ss.imbue(std::locale("C"));

  // std::cout
  //   << "TrackPgDao::tracked_locations_result query: "
  //   << qp << '\n';
  // First do query to get total count
  ss <<
    "SELECT COUNT(*) "
    "FROM location ";
  append_location_query_where_clause(ss, tx, qp);
  result r = tx.exec(ss.str());
  if (!r.empty())
    r[0][0].to(retval.total_count);

  auto offset = qp.page_offset;
  // Adjust the offset so that the maximum returned discards the oldest
  if (maximum >= 0 && retval.total_count > maximum)
    offset = retval.total_count - maximum;

  // Then fetch the page
  std::ostringstream os;
  os.imbue(std::locale("C"));
  os <<
    "SELECT id, ST_X(geog::geometry) as lng, ST_Y(geog::geometry) as lat, "
    "time, hdop, altitude, speed, bearing, sat, provider, battery, note "
    "FROM location ";
  append_location_query_where_clause(os, tx, qp);
  os <<
    " ORDER BY "
    "time "
     << (qp.order == location_search_query_params::ascending ? "ASC" : "DESC")
     << ", "

    "id "
     << (qp.order == location_search_query_params::ascending ? "ASC" : "DESC")
     << ' ';
  // Then fetch results
  if (offset >= 0)
    os << "OFFSET " << offset << ' ';

  if (qp.page_size >= 0)
    os << "LIMIT " << qp.page_size;

  // std::cout << "SQL: " << os.str() << '\n';

  r = tx.exec(os.str());
  for (result::const_iterator i = r.begin(); i != r.end(); ++i) {
    tracked_location loc;
    loc.id = i["id"].as<long>();
    i["lng"].to(loc.longitude);
    i["lat"].to(loc.latitude);
    std::string date_str;
    loc.time_point = dao_helper::convert_libpq_date_tz(i["time"].as<std::string>());
    if (!i["hdop"].is_null())
      loc.hdop = i["hdop"].as<float>();
    if (!i["altitude"].is_null())
      loc.altitude = i["altitude"].as<double>();
    if (!i["speed"].is_null())
      loc.speed = i["speed"].as<float>();
    if (!i["bearing"].is_null())
      loc.bearing = i["bearing"].as<double>();
    if (!i["sat"].is_null())
      loc.satellite_count = i["sat"].as<int>();
    if (!i["provider"].is_null())
      loc.provider = i["provider"].as<std::optional<std::string>>();
    if (!i["battery"].is_null())
      loc.battery = i["battery"].as<float>();
    if (!i["note"].is_null())
      loc.note = i["note"].as<std::optional<std::string>>();
    retval.locations.push_back(loc);
  }
  tx.commit();
  return retval;
}

/**
 * Fetches informaton about what is being shared by nickname to user_id.
 *
 * \param shared_by_nickname the nickname of the sharing user.
 *
 * \param shared_to_user_id the user ID of the user the data is being shared
 * to.
 *
 * \return a std::optional, the first element set to true if the query was
 * successful.  The second element constains details of the location sharing
 * for the requested sharing nickname.
 */
std::optional<TrackPgDao::location_share_details>
    TrackPgDao::get_tracked_location_share_details_by_sharee(
        std::string shared_by_nickname,
        std::string shared_to_user_id) const
{
  work tx(*connection);
  const std::string sql =
    "SELECT recent_minutes, max_minutes, active, shared_by_id, shared_to_id "
    "FROM location_sharing ls "
    "JOIN usertable u1 ON u1.id=ls.shared_by_id "
    // "JOIN usertable u2 ON u2.id=ls.shared_to_id "
    "WHERE u1.nickname='" +
    tx.esc(shared_by_nickname) + "' AND ls.active=true AND "
    "ls.shared_to_id='" + tx.esc(shared_to_user_id) + '\'';

  result r = tx.exec(sql);
  std::optional<TrackPgDao::location_share_details> retval;
  if(!r.empty()) {
    location_share_details share_details;
    if (!r[0]["recent_minutes"].is_null())
      share_details.recent_minutes = r[0]["recent_minutes"].as<int>();
    if (!r[0]["max_minutes"].is_null())
      share_details.max_minutes = r[0]["max_minutes"].as<int>();
    if (!r[0]["active"].is_null())
      share_details.active = r[0]["active"].as<bool>();
    r[0]["shared_by_id"].to(share_details.shared_by_id);
    r[0]["shared_to_id"].to(share_details.shared_to_id);
    retval = share_details;
  }
  tx.commit();
  return retval;
}

std::optional<TrackPgDao::location_share_details>
    TrackPgDao::get_tracked_location_share_details_by_sharer(
        std::string shared_to_nickname,
        std::string shared_by_user_id) const
{
  try {
    work tx(*connection);
    const std::string sql =
      "SELECT recent_minutes, max_minutes, active, shared_by_id, shared_to_id "
      "FROM location_sharing ls "
      "JOIN usertable u ON u.id=ls.shared_to_id "
      "WHERE u.nickname=$1 AND ls.shared_by_id=$2";
    auto r = tx.exec_params1(
        sql,
        shared_to_nickname,
        shared_by_user_id);
    auto retval = std::optional<location_share_details>();
    location_share_details share_details;
    if (!r["recent_minutes"].is_null())
      share_details.recent_minutes = r["recent_minutes"].as<int>();
    if (!r["max_minutes"].is_null())
      share_details.max_minutes = r["max_minutes"].as<int>();
    if (!r["active"].is_null())
      share_details.active = r["active"].as<bool>();
    r["shared_by_id"].to(share_details.shared_by_id);
    r["shared_to_id"].to(share_details.shared_to_id);
    tx.commit();
    retval = share_details;
    return retval;
  } catch (const std::exception &e) {
    std::cerr << "Exception in "
      "TrackPgDao::get_tracked_location_share_details_by_sharer(): "
              << e.what() << '\n';
    throw;
  }
}

/**
 * Returns whether locations have been recorded since the specified
 * `min_id_threshold`.  The ID is the database unique identifier for a tracked
 * location.
 *
 * This is intended to be used to check whether there are further updates and
 * the caller can then choose to fetch the additional updates etc.
 *
 * \param user_id of the current user.
 *
 * \param nickname the nickname of the user whose locations are being checked.
 *
 * \param min_id_threshold the database ID of the tracked location to be
 * excluded as well as those of a lower value.
 *
 * \return true if there are any tracked locations with an ID higher than the
 * specified threshold and belonging to the passed nickname.
 */
bool TrackPgDao::check_new_locations_available(
    std::string user_id,
    std::string nickname,
    long min_id_threshold)
{
  bool retval = false;
  work tx(*connection);
  try {
    std::ostringstream sql;
    if (nickname.empty()) {
      sql <<
        "SELECT COUNT(*) FROM location loc "
        "WHERE loc.user_id=$1 AND loc.id > $2";
      auto r = tx.exec_params1(
          sql.str(),
          user_id,
          min_id_threshold);
      retval = r[0].as<long>() > 0;
    } else {
      sql <<
        "SELECT COUNT(*) FROM location loc "
        "JOIN location_sharing sh ON loc.user_id=sh.shared_by_id "
        "WHERE user_id=(SELECT id FROM usertable WHERE nickname = $2) "
        "AND sh.shared_to_id=$1 AND loc.id > $3";
      auto r = tx.exec_params1(
          sql.str(),
          user_id,
          nickname,
          min_id_threshold);
      retval = r[0].as<long>() > 0;
    }
    // std::cout << "SQL: " << sql.str() << '\n';
    tx.commit();
    return retval;
  } catch (const std::exception &e) {
    std::cerr << "Exception checking for new location updates: "
              << e.what();
    throw;
  }
}

std::string TrackPgDao::get_nickname_for_user_id(
    work &tx, std::string user_id)
{
  const auto r = tx.exec_params1(
      "SELECT nickname FROM usertable WHERE id=$1",
      user_id);
  return r["nickname"].as<std::string>();
}

std::string TrackPgDao::get_nickname_for_user_id(std::string user_id)
{
  work tx(*connection);
  auto retval = get_nickname_for_user_id(tx, user_id);
  tx.commit();
  return retval;
}

/**
 * Returns whether locations have been recorded since the specified
 * `min_id_threshold`.  The ID is the database unique identifier for a tracked
 * location.
 *
 * This is intended to be used to check whether there are further updates and
 * the caller can then choose to fetch the additional updates etc.
 *
 * \param user_id of the current user.
 *
 * \param criteria an object containg the nickname of the user whose locations
 * are being checked and optionally the min_id_threshold the database ID of the
 * tracked location to be excluded as well as those of a lower value.  The
 * passed criteria is updated to indicate whether there are updates available.
 *
 */
void TrackPgDao::check_new_locations_available(
    std::string user_id,
    std::chrono::system_clock::time_point since,
    std::vector<location_update_check> &criteria)
{
  work tx(*connection);
  try {
    const std::string user_nickname = get_nickname_for_user_id(tx, user_id);
    const std::string ps_name = "new-location-checks-nickname";
    connection->prepare(
        ps_name,
        "SELECT COUNT(*) FROM location loc "
        "JOIN location_sharing sh ON loc.user_id=sh.shared_by_id "
        "WHERE user_id=(SELECT id FROM usertable WHERE nickname=$2) "
        "AND sh.shared_to_id=$1 AND (loc.id > $3 OR $3 IS NULL) AND time >= $4"
      );
    const DateTime from(since);
    for (auto &c : criteria) {
      if (c.nickname == user_nickname) {
        auto r = tx.exec_params1(
            "SELECT COUNT(*) FROM location loc "
            "WHERE loc.user_id=$1 AND (loc.id > $2 OR $2 IS NULL) AND time >= $3",
            user_id,
            c.min_threshold_id,
            from.get_time_as_iso8601_gmt());
        c.update_available = r[0].as<long>() > 0;
      } else {
        auto r = tx.exec_prepared1(
            ps_name,
            user_id,
            c.nickname,
            c.min_threshold_id,
            from.get_time_as_iso8601_gmt());
        c.update_available = r[0].as<long>() > 0;
      }
    }
    tx.commit();
    connection->unprepare(ps_name);
  } catch (const std::exception &e) {
    std::cerr << "Exception checking for user date since datetime : "
              << e.what();
    throw;
  }
}

std::optional<std::time_t>
    TrackPgDao::get_most_recent_location_time(std::string shared_by_id) const
{
  work tx(*connection);
  result r = tx.exec(
      "SELECT max(\"time\") AS \"time\" FROM location WHERE user_id='" +
      tx.esc(shared_by_id) + '\''
    );
  std::optional<std::time_t> retval;
  const bool success = !r.empty();
  if (success) {
    std::string date_str;
    const bool ok = r[0][0].to(date_str);
    if (ok)
      retval = dao_helper::convert_libpq_date(date_str);
    return retval;
  }
  tx.commit();
  return retval;
}

/**
 * Reflects the constraints based on both how long before now, and how long
 * before the most recently shared track location another user can see the
 * shared track locations for the specified user ID.
 *
 * \param shared_by_id the ID of the user sharing their track locations
 *
 * \param date_from the initially requested start period
 *
 * \param date_to the initially requested end period
 *
 * \param max_minutes the maximum time before now that the target user can
 * view track locations
 *
 * \param recent_minutes the most recently tracked location period limitation
 *
 * \return the date range that is valid for the target user to view the data
 * for.
 */
TrackPgDao::date_range TrackPgDao::constrain_shared_location_dates(
    std::string shared_by_id,
    std::time_t date_from,
    std::time_t date_to,
    std::optional<int> max_minutes,
    std::optional<int> recent_minutes) const
{
  date_range retval;
  auto now = std::chrono::system_clock::now();
  std::optional<std::time_t> earliest_possible;
  std::optional<std::time_t> most_recent_from;

  // If max_minutes is set, calculate the earliest possible date as max
  // minutes before now
  if (max_minutes.has_value() && max_minutes.value() > 0) {
    // std::cout << "max_minutes: " << max_minutes.second << '\n';
    std::chrono::minutes max_duration(max_minutes.value());
    auto r = now - max_duration;
    earliest_possible = std::chrono::system_clock::to_time_t(r);
  //   DateTime ep(earliest_possible.second);
  //   std::cout << "Based on max_minutes, earliest possible date is " << ep << '\n';
  // } else {
  //   std::cout << "max_minutes is not set\n";
  }

  // If recent_minutes is set, calculate the most_recent_from based on the
  // most recently recorded location.
  if (recent_minutes.has_value() && recent_minutes.value() > 0) {
    // std::cout << "recent_minutes: " << recent_minutes.second << '\n';
    std::optional<std::time_t> latest_time = get_most_recent_location_time(shared_by_id);
    // if (latest_time.first) {
    // std::cout << "Most recent shared date: " << latest_time.second << '\n';
    // }
    if (latest_time.has_value()) {
      std::chrono::minutes recent_duration(recent_minutes.value());
      auto latest = std::chrono::system_clock::from_time_t(latest_time.value());
      auto r = latest - recent_duration;
      most_recent_from = std::chrono::system_clock::to_time_t(r);
      // DateTime mr(most_recent_from.second);
      // std::cout << "Based on recent_minutes, earliest possible date is " << mr << '\n';
    } else {
      // std::cout << "recent_minutes is not set\n";
      most_recent_from = earliest_possible;
    }
  }

  // If earliest date (based on minutes before now) is more recent than the
  // date based on most recently recorded location, the most recently recorded
  // location criteria takes precedence.
  if (most_recent_from.has_value() &&
      (!earliest_possible.has_value() ||
       earliest_possible.value() < most_recent_from.value()))
    earliest_possible = most_recent_from;

  // earliest possible now reflects the earliest date the user can see the
  // shared track location for, by either criteria.

  // If query from_date is earlier than the earliest allowed, set it to the
  // earliest allowed.
  if (earliest_possible.has_value() &&
      date_from < earliest_possible.value()) {
    date_from = earliest_possible.value();
    // DateTime df(date_from);
    // std::cout << "Adjusted date from to " << df << '\n';
  }

  // Make sure date_from is before date_to
  retval.from = date_from;
  retval.to = date_to;
  if (retval.from > retval.to) {
    retval.to = retval.from;
  }
  return retval;
}

TrackPgDao::tracked_locations_result TrackPgDao::get_shared_tracked_locations(
    const location_search_query_params& qp, int maximum) const
{
  tracked_locations_result retval;

  // Information about what is being shared by nickname to user_id
  std::optional<location_share_details> sharing_criteria =
    get_tracked_location_share_details_by_sharee(
        qp.nickname,
        qp.user_id);

  if (sharing_criteria.has_value() &&
      sharing_criteria.value().active.has_value() &&
      sharing_criteria.value().active.value()) {

    date_range period = constrain_shared_location_dates(
        sharing_criteria.value().shared_by_id,
        qp.date_from,
        qp.date_to,
        sharing_criteria.value().max_minutes,
        sharing_criteria.value().recent_minutes);

    location_search_query_params modified_params = qp;
    modified_params.user_id = sharing_criteria.value().shared_by_id;
    modified_params.date_from = period.from;
    modified_params.date_to = period.to;

    // std::cout << "Performing tracked location search with: "
    //           << modified_params << '\n';
    retval = get_tracked_locations_for_user(modified_params, maximum);
  } else {
    // Search failed.  Set some sensible values.
    retval.date_from = qp.date_from;
    retval.date_to = qp.date_to;
  }
  return retval;
}

TrackPgDao::tracked_locations_result TrackPgDao::get_tracked_locations(
    const location_search_query_params& location_search,
    int maximum,
    bool fill_distance_and_elevation_values) const {

  TrackPgDao::tracked_locations_result retval =
    (location_search.nickname.empty()) ?
    // std::cout << "Nickname is empty, searching for this user's tracks\n";
    get_tracked_locations_for_user(location_search, maximum) :
    // std::cout << "Nickname is not empty, searching for the tracks belonging "
    get_shared_tracked_locations(location_search, maximum);

#ifdef HAVE_GDAL
  if (fill_distance_and_elevation_values) {
    elevation_service->fill_elevations(
        retval.locations.begin(),
        retval.locations.end());
  }
#endif

  return retval;
}

TrackPgDao::nickname_result TrackPgDao::get_nicknames(std::string user_id)
{
  nickname_result retval;
  work tx(*connection);
  result r = tx.exec(
      "SELECT nickname FROM usertable WHERE id = '" + tx.esc(user_id) + "'");

  if (!r.empty())
    r[0]["nickname"].to(retval.nickname);

  r = tx.exec(
      "SELECT u2.nickname FROM usertable u "
      "JOIN location_sharing ls ON u.id=ls.shared_to_id "
      "JOIN usertable u2 ON ls.shared_by_id=u2.id "
      "WHERE ls.active=true AND u.id='" + tx.esc(user_id) + "' "
      "ORDER BY u2.nickname");
  for (result::const_iterator i = r.begin(); i != r.end(); ++i) {
    std::string sharing_nickname;
    i["nickname"].to(sharing_nickname);
    retval.nicknames.push_back(sharing_nickname);
  }
  tx.commit();
  return retval;
}

std::string TrackPgDao::get_user_id_by_uuid(std::string uuid)
{
  std::string user_id;
  work tx(*connection);
  auto r = tx.exec_params(
      "SELECT id, nickname FROM usertable WHERE uuid = $1",
      uuid);
  tx.commit();
  if (!r.empty())
    user_id = r[0]["id"].as<std::string>();
  return user_id;
}

std::string TrackPgDao::get_logging_uuid_by_user_id(std::string user_id)
{
  std::string logging_uuid;
  work tx(*connection);
  auto r = tx.exec_params(
      "SELECT uuid FROM usertable WHERE id = $1",
      user_id);
  tx.commit();
  if (!r.empty())
    logging_uuid = r[0]["uuid"].as<std::string>();
  return logging_uuid;
}

// \throws pqxx::unexpected_rows
std::string TrackPgDao::get_user_id_by_nickname(std::string nickname)
{
  // try {
    work tx(*connection);
    auto r = tx.exec_params1(
        "SELECT id FROM usertable WHERE nickname=$1",
        nickname);
    tx.commit();
    if (!r.empty())
      return r.front().as<std::string>();
  // } catch (const std::exception &e) {
  //   std::cerr << "Exception in "
  //     "TrackPgDao::get_user_id_by_nickname(): "
  //             << e.what() << '\n';
  //   throw;
  // }
  return "";
}

void TrackPgDao::save_logging_uuid(std::string user_id,
                                   std::string logging_uuid)
{
  work tx(*connection);
  tx.exec_params("UPDATE usertable SET uuid=$2 WHERE id=$1",
                 user_id,
                 logging_uuid);
  tx.commit();
}

void TrackPgDao::save_tracked_location(
    const TrackPgDao::tracked_location_query_params& qp)
{
  work tx(*connection);
  const DateTime dt(qp.time_point);
  tx.exec_params(
      "INSERT INTO location ("
      "user_id, geog, time, hdop, altitude, speed, bearing, sat, "
      "provider, battery, note) "
      "VALUES($1, ST_SetSRID(ST_POINT($2, $3),4326), "
      "$4, $5, $6, $7, $8, $9, $10, $11, $12)",
      qp.user_id, qp.longitude, qp.latitude,
      dt.get_time_as_iso8601_gmt(),
      qp.hdop,
      qp.altitude,
      qp.speed,
      qp.bearing,
      qp.satellite_count,
      qp.provider,
      qp.battery,
      qp.note);
  tx.commit();
}

void TrackPgDao::save(const location_share_details& share)
{
  try {
    work tx(*connection);
    tx.exec_params(
        "INSERT INTO location_sharing "
        "(shared_by_id, shared_to_id, recent_minutes, max_minutes, active) "
        "VALUES ($1, $2, $3, $4, $5) "
        "ON CONFLICT (shared_by_id, shared_to_id) DO UPDATE "
        "SET recent_minutes=$3, max_minutes=$4, active=$5",
        share.shared_by_id,
        share.shared_to_id,
        share.recent_minutes,
        share.max_minutes,
        share.active
      );
    tx.commit();
  } catch (const std::exception& e) {
    std::cerr << "Exception saving location share: "
              << e.what() << '\n';
    throw;
  }
}

/**
 * Gets the most recently saved configuration file used by the TripLogger iOS
 * app.
 */
TrackPgDao::triplogger_configuration TrackPgDao::get_triplogger_configuration(
    std::string user_id)
{
  triplogger_configuration c;
  try {
    work tx(*connection);
    auto r = tx.exec_params(
        "SELECT uuid, tl_settings FROM usertable WHERE id=$1",
        user_id);
    if (!r.empty()) {
      c.uuid = r[0]["uuid"].as<std::string>();
      const auto f = r[0]["tl_settings"];
      if (!f.is_null())
        c.tl_settings = f.as<std::string>();
    }
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception fetching tracklogger configuration: "
              << e.what() << '\n';
    throw;
  }
  return c;
}

void TrackPgDao::save(std::string user_id,
                      const triplogger_configuration & tl_config)
{
  try {
    work tx(*connection);
    tx.exec_params("UPDATE usertable SET tl_settings=$2 WHERE id=$1",
                    user_id,
                    tl_config.tl_settings);
    tx.commit();
  } catch (const std::exception& e) {
    std::cerr << "Exception saving TripLogger settings: "
              << e.what() << '\n';
    throw;
  }
}

long TrackPgDao::get_track_sharing_count_by_user_id(std::string user_id)
{
  try {
    work tx(*connection);
    auto r = tx.exec_params1("SELECT COUNT(*) FROM location_sharing WHERE shared_by_id=$1",
                             user_id);
    tx.commit();
    return r.front().as<long>();
  } catch (const std::exception &e) {
    std::cerr << "Exception in "
      "TrackPgDao::get_track_sharing_count_by_user_id(): "
              << e.what() << '\n';
    throw;
  }
}

std::vector<TrackPgDao::track_share> TrackPgDao::get_track_sharing_by_user_id(
    std::string user_id,
    std::uint32_t offset,
    int limit)
{
  std::string sql =
    "SELECT u.nickname, ls.recent_minutes, ls.max_minutes, ls.active "
    "FROM location_sharing ls JOIN usertable u ON u.id=ls.shared_to_id "
    "WHERE ls.shared_by_id=$1 ORDER BY u.nickname OFFSET $2 LIMIT $3";
  // std::cout << "Offset: " << offset << ", limit: " << limit << '\n';
  // std::cout << "sql: " << sql << '\n';
  work tx(*connection);
  auto r = tx.exec_params(sql,
                          user_id,
                          offset,
                          limit);
  std::vector<track_share> retval;
  for (result::const_iterator i = r.begin(); i != r.end(); ++i) {
    track_share ts;
    if (!i["nickname"].is_null())
      ts.nickname = i["nickname"].as<std::string>();
    if (!i["recent_minutes"].is_null())
      ts.recent_minutes = i["recent_minutes"].as<int>();
    if (!i["max_minutes"].is_null())
      ts.max_minutes = i["max_minutes"].as<int>();
    if (!i["active"].is_null())
      ts.active = i["active"].as<bool>();
    retval.push_back(ts);
  }
  tx.commit();
  return retval;
}

void TrackPgDao::delete_track_shares(std::string user_id,
                                     std::vector<std::string> nicknames)
{
  if (nicknames.empty())
    return;
  try {
    const std::string ps_name = "delete-track-shares";
    connection->prepare(
        ps_name,
        "DELETE FROM location_sharing "
        "WHERE shared_by_id=$1 AND shared_to_id="
        "(SELECT u.id FROM usertable u WHERE u.nickname=$2)");
    work tx(*connection);
    for (auto const& nickname : nicknames) {
      // std::cout << "Deleting " << nickname << '\n';
      tx.exec_prepared(
          ps_name,
          user_id,
          nickname);
    }
    tx.commit();
    connection->unprepare(ps_name);
  } catch (const std::exception &e) {
    std::cerr << "Exception deleting track shares: "
              << e.what() << '\n';
    throw;
  }
}

void TrackPgDao::activate_track_shares(std::string user_id,
                                       std::vector<std::string> nicknames,
                                       bool activate)
{
  if (nicknames.empty())
    return;
  try {
    const std::string ps_name = "activate-track-shares";
    connection->prepare(
        ps_name,
        "UPDATE location_sharing SET active=$3 "
        "WHERE shared_by_id=$1 AND shared_to_id="
        "(SELECT u.id FROM usertable u WHERE u.nickname=$2)");
    work tx(*connection);
    for (auto const& nickname : nicknames) {
      // std::cout << "Updating " << nickname << " " << activate << '\n';
      tx.exec_prepared(
          ps_name,
          user_id,
          nickname,
          activate);
    }
    tx.commit();
    connection->unprepare(ps_name);
  } catch (const std::exception &e) {
    std::cerr << "Exception activating track shares: "
              << e.what() << '\n';
    throw;
  }
}
