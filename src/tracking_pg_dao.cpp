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
#include "tracking_pg_dao.hpp"
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
        const location_search_query_params& qp) const
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
  if (qp.page_offset >= 0)
    os << "OFFSET " << qp.page_offset << ' ';

  if (qp.page_size >= 0)
    os << "LIMIT " << qp.page_size;

  // std::cout << "SQL: " << os.str() << '\n';

  r = tx.exec(os.str());
  for (result::const_iterator i = r.begin(); i != r.end(); ++i) {
    tracked_location loc;
    i["id"].to(loc.id);
    i["lng"].to(loc.longitude);
    i["lat"].to(loc.latitude);
    std::string date_str;
    loc.time = dao_helper::convert_libpq_date(i["time"].as<std::string>());
    loc.hdop.first = i["hdop"].to(loc.hdop.second);
    loc.altitude.first = i["altitude"].to(loc.altitude.second);
    loc.speed.first = i["speed"].to(loc.speed.second);
    loc.bearing.first = i["bearing"].to(loc.bearing.second);
    loc.satellite_count.first = i["sat"].to(loc.satellite_count.second);
    // This fails to compile with C++17, with a not-assignable message, but I
    // don't see why it isn't assignable.
    // <std::optional<int>> satellite_count  = satellite.as<std::optional<int>>();
    i["provider"].to(loc.provider);
    loc.battery.first = i["battery"].to(loc.battery.second);
    i["note"].to(loc.note);
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
 * \return a std::pair, the first element set to true if the query was
 * successful.  The second element constains details of the location sharing
 * for the requested sharing nickname.
 */
std::pair<bool, TrackPgDao::location_share_details>
    TrackPgDao::get_tracked_location_share_details(
        std::string shared_by_nickname,
        std::string shared_to_user_id) const
{
  work tx(*connection);
  const std::string sql =
    "SELECT recent_minutes, max_minutes, active, shared_by_id "
    "FROM location_sharing ls "
    "JOIN usertable u1 ON u1.id=ls.shared_by_id "
    // "JOIN usertable u2 ON u2.id=ls.shared_to_id "
    "WHERE u1.nickname='" +
    tx.esc(shared_by_nickname) + "' AND "
    "ls.shared_to_id='" + tx.esc(shared_to_user_id) + '\'';

  result r = tx.exec(sql);
  location_share_details share_details;
  auto retval = std::make_pair(false, share_details);
  retval.first = !r.empty();
  if (retval.first) {
    retval.second.recent_minutes.first =
      r[0]["recent_minutes"].to(retval.second.recent_minutes.second);
    retval.second.max_minutes.first =
      r[0]["max_minutes"].to(retval.second.max_minutes.second);
    retval.second.active.first =
      r[0]["active"].to(retval.second.active.second);
      r[0]["shared_by_id"].to(retval.second.shared_by_id);
  }
  tx.commit();
  return retval;
}

std::pair<bool, std::time_t>
    TrackPgDao::get_most_recent_location_time(std::string shared_by_id) const
{
  work tx(*connection);
  result r = tx.exec(
      "SELECT max(\"time\") AS \"time\" FROM location WHERE user_id='" +
      tx.esc(shared_by_id) + '\''
    );
  const bool success = !r.empty();
  if (success) {
    std::string date_str;
    const bool success = r[0][0].to(date_str);
    return std::make_pair(success, dao_helper::convert_libpq_date(date_str));
  }
  tx.commit();
  return std::make_pair(false, 0);
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
    std::pair<bool, int> max_minutes,
    std::pair<bool, int> recent_minutes) const
{
  date_range retval;
  auto now = std::chrono::system_clock::now();
  std::pair<bool, std::time_t> earliest_possible;
  std::pair<bool, std::time_t> most_recent_from;

  // If max_minutes is set, calculate the earliest possible date as max
  // minutes before now
  if (max_minutes.first && max_minutes.second > 0) {
    // std::cout << "max_minutes: " << max_minutes.second << '\n';
    std::chrono::minutes max_duration(max_minutes.second);
    auto r = now - max_duration;
    earliest_possible = std::make_pair(true, std::chrono::system_clock::to_time_t(r));
  //   DateTime ep(earliest_possible.second);
  //   std::cout << "Based on max_minutes, earliest possible date is " << ep << '\n';
  // } else {
  //   std::cout << "max_minutes is not set\n";
  }

  // If recent_minutes is set, calculate the most_recent_from based on the
  // most recently recorded location.
  if (recent_minutes.first && recent_minutes.second > 0) {
    // std::cout << "recent_minutes: " << recent_minutes.second << '\n';
    std::pair<bool, std::time_t> latest_time = get_most_recent_location_time(shared_by_id);
    // if (latest_time.first) {
    // std::cout << "Most recent shared date: " << latest_time.second << '\n';
    // }
    if (latest_time.first) {
      std::chrono::minutes recent_duration(recent_minutes.second);
      auto latest = std::chrono::system_clock::from_time_t(latest_time.second);
      auto r = latest - recent_duration;
      most_recent_from = std::make_pair(true, std::chrono::system_clock::to_time_t(r));
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
  if (most_recent_from.first &&
      (!earliest_possible.first ||
       earliest_possible.second < most_recent_from.second))
    earliest_possible = most_recent_from;

  // earliest possible now reflects the earliest date the user can see the
  // shared track location for, by either criteria.

  // If query from_date is earlier than the earliest allowed, set it to the
  // earliest allowed.
  if (earliest_possible.first &&
      date_from < earliest_possible.second) {
    date_from = earliest_possible.second;
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
    const location_search_query_params& qp) const
{
  tracked_locations_result retval;

  // Information about what is being shared by nickname to user_id
  std::pair<bool, location_share_details> sharing_criteria =
    get_tracked_location_share_details(
        qp.nickname,
        qp.user_id);

  if (sharing_criteria.first &&
      sharing_criteria.second.active.first &&
      sharing_criteria.second.active.second) {

    date_range period = constrain_shared_location_dates(
        sharing_criteria.second.shared_by_id,
        qp.date_from,
        qp.date_to,
        sharing_criteria.second.max_minutes,
        sharing_criteria.second.recent_minutes);

    location_search_query_params modified_params = qp;
    modified_params.user_id = sharing_criteria.second.shared_by_id;
    modified_params.date_from = period.from;
    modified_params.date_to = period.to;

    // std::cout << "Performing tracked location search with: "
    //           << modified_params << '\n';
    retval = get_tracked_locations_for_user(modified_params);
  }
  return retval;
}

TrackPgDao::tracked_locations_result TrackPgDao::get_tracked_locations(
    const location_search_query_params& location_search,
    bool fill_distance_and_elevation_values) const {

  if (fill_distance_and_elevation_values) {
    // TODO implement fill_distance_and_elevation_values
    throw std::runtime_error("Filling in distance and elevation values has not "
                             "been implemented");
  }

  if (location_search.nickname.empty()) {
    // std::cout << "Nickname is empty, searching for this user's tracks\n";
    return get_tracked_locations_for_user(location_search);
  } else {
    // std::cout << "Nickname is not empty, searching for the tracks belonging "
    // "to \"" << location_search.nickname << "\"\n";
    return get_shared_tracked_locations(location_search);
  }
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

