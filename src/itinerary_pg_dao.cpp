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
#include "itinerary_pg_dao.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include "../trip-server-common/src/dao_helper.hpp"
#include <algorithm>
#include <memory>
#include <vector>
#include <stdexcept>

using namespace fdsd::trip;
using namespace fdsd::utils;
using namespace pqxx;
using json = nlohmann::json;

namespace fdsd {
namespace trip {

void to_json(nlohmann::json& j, const ItineraryPgDao::selected_feature_ids& ids)
{
  ItineraryPgDao::selected_feature_ids::to_json(j, ids);
}

void from_json(const nlohmann::json& j,  ItineraryPgDao::selected_feature_ids& ids)
{
  ItineraryPgDao::selected_feature_ids::from_json(j, ids);
}

void to_json(nlohmann::json& j, const ItineraryPgDao::path_color& p)
{
  ItineraryPgDao::path_color::to_json(j, p);
}

void from_json(const nlohmann::json& j,  ItineraryPgDao::path_color& p)
{
  ItineraryPgDao::path_color::from_json(j, p);
}

} // namespace trip
} // namespace fdsd

void ItineraryPgDao::selected_feature_ids::to_json(
    nlohmann::json& j,
    const ItineraryPgDao::selected_feature_ids& ids)
{
  j = json{
    {"routes", ids.routes},
    {"tracks", ids.tracks},
    {"waypoints", ids.waypoints}
  };
}

void ItineraryPgDao::selected_feature_ids::from_json(
    const nlohmann::json& j,
    ItineraryPgDao::selected_feature_ids& ids)
{
  j.at("routes").get_to(ids.routes);
  j.at("tracks").get_to(ids.tracks);
  j.at("waypoints").get_to(ids.waypoints);
}

void ItineraryPgDao::path_color::to_json(
    nlohmann::json& j,
    const ItineraryPgDao::path_color& p)
{
  j = json{
    {"key", p.key},
    {"description", p.description},
    {"html_code", p.html_code}
  };
}

void ItineraryPgDao::path_color::from_json(
    const nlohmann::json& j,
    ItineraryPgDao::path_color& p)
{
  j.at("key").get_to(p.key);
  j.at("description").get_to(p.description);
  j.at("html_code").get_to(p.html_code);
}

void ItineraryPgDao::path_summary::from_geojson_properties(
    const nlohmann::json& properties,
    ItineraryPgDao::path_summary &path)
{
  const auto name_iter = properties.find("name");
  if (name_iter != properties.cend()) {
    std::string s;
    name_iter->get_to(s);
    path.name = s;
  }
  const auto html_color_iter = properties.find("html_color_code");
  if (html_color_iter != properties.cend()) {
    std::string s;
    html_color_iter->get_to(s);
    path.html_code = s;
  }
  const auto color_key_iter = properties.find("color_code");
  if (color_key_iter != properties.cend()) {
    std::string s;
    color_key_iter->get_to(s);
    path.color_key = s;
  }
}

ItineraryPgDao::route::route(const track &t) : path_summary(t), points()
{
  id = std::optional<long>();
  // We lose separate segments from the track
  for (const auto &ts : t.segments) {
    for (const auto &tp : ts.points) {
      // We lose time and hdop from the track point
      route_point rp(tp);
      rp.id = std::optional<long>();
      points.push_back(rp);
    }
  }
  calculate_statistics();
}

void ItineraryPgDao::route::calculate_statistics()
{
  GeoStatistics geo_stats;
  geo_stats.add_path(points.begin(), points.end());
  lowest = geo_stats.get_lowest();
  highest = geo_stats.get_highest();
  ascent = geo_stats.get_ascent();
  descent = geo_stats.get_descent();
  distance = geo_stats.get_distance();
}

ItineraryPgDao::track::track(const route &r) : path_summary(r), segments()
{
  // Some applications expect track points to have a time value, so start from
  // 1901-12-13T20:45:52Z (earliest timestamp avoiding any issues with systems
  // still running 32-bit integers for Unix time)
  auto time = std::chrono::system_clock::from_time_t(-2147483648);
  // One second intervals between points
  const auto interval = std::chrono::seconds(1);
  id = std::optional<long>();
  // Single segment for the track
  track_segment ts;
  for (const auto &rp : r.points) {
    // We lose name, comment, description and symbol from the route point
    track_point tp(rp);
    tp.id = std::optional<long>();
    tp.time = time;
    time += interval;
    ts.points.push_back(tp);
  }
  segments.push_back(ts);
  calculate_statistics();
}

void ItineraryPgDao::track::calculate_statistics()
{
  GeoStatistics geo_stats;
  for (auto &segment : segments) {
    geo_stats.add_path(segment.points.begin(), segment.points.end());
    GeoStatistics segment_stats;
    segment_stats.add_path(segment.points.begin(), segment.points.end());
    segment.lowest = segment_stats.get_lowest();
    segment.highest = segment_stats.get_highest();
    segment.ascent = segment_stats.get_ascent();
    segment.descent = segment_stats.get_descent();
    segment.distance = segment_stats.get_distance();
  }
  lowest = geo_stats.get_lowest();
  highest = geo_stats.get_highest();
  ascent = geo_stats.get_ascent();
  descent = geo_stats.get_descent();
  distance = geo_stats.get_distance();
}

void ItineraryPgDao::track::calculate_speed_and_bearing_values()
{
  std::unique_ptr<track_point> tp;
  for (auto &ts : segments)
    for (auto p = ts.points.begin(); p != ts.points.end(); p++) {
      if (tp) {
        p->calculate_bearing(*tp);
        p->calculate_speed(*tp);
      }
      tp = std::unique_ptr<track_point>(new track_point(*p));
    }
}

std::optional<double> ItineraryPgDao::track::calculate_maximum_speed() const
{
  std::optional<double> retval;
  for (const auto &ts : segments)
    for (const auto &p : ts.points)
      if (p.speed.has_value()) {
        if (retval.has_value())
          retval = std::max(retval.value(), p.speed.value());
        else
          retval = p.speed.value();
      }
  return retval;
}

ItineraryPgDao::track_point::track_point(
    const TrackPgDao::tracked_location &loc) : point_info(loc), speed()
{
  time = loc.time_point;
  hdop = loc.hdop;
}

void ItineraryPgDao::point_info::calculate_bearing(const point_info &previous)
{
  bearing = GeoUtils::bearing_to_azimuth(previous, *this);
}

void ItineraryPgDao::track_point::calculate_speed(const track_point &previous)
{
  if (time.has_value() && previous.time.has_value()) {
    const double distance = GeoUtils::distance(previous, *this);
    std::chrono::duration<double, std::milli> diff =
      time.value() - previous.time.value();
    const double hours =
      std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() / 3600000.0;
    speed = distance / hours;
  }
}

long ItineraryPgDao::get_itineraries_count(
    std::string user_id)
{
  try {
    work tx(*connection);
    auto r = tx.exec_params1(
        "SELECT sum(count) FROM (SELECT count(*) FROM itinerary i JOIN usertable u ON i.user_id=u.id WHERE i.archived != true AND u.id=$1 UNION ALL SELECT count(*) FROM itinerary i2 JOIN usertable u3 ON u3.id=i2.user_id JOIN itinerary_sharing s ON i2.id=s.itinerary_id JOIN usertable u2 ON u2.id=s.shared_to_id WHERE i2.archived != true AND s.active=true AND u2.id=$1) as q",
        user_id
      );
    tx.commit();
    return r[0].as<long>();
  } catch (const std::exception &e) {
    std::cerr << "Exception executing query to fetch itinerary count: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::itinerary_summary> ItineraryPgDao::get_itineraries(
    std::string user_id,
    std::uint32_t offset,
    int limit)
{
  try {
    work tx(*connection);
    auto result = tx.exec_params(
        "SELECT i.id, i.start, i.finish, i.title, null AS nickname, (SELECT true FROM (SELECT DISTINCT itinerary_id FROM itinerary_sharing xx WHERE xx.itinerary_id=i.id AND active=true) as q) AS shared FROM itinerary i JOIN usertable u ON i.user_id=u.id WHERE i.archived != true AND u.id=$1 UNION SELECT i2.id, i2.start, i2.finish, i2.title, u3.nickname, false AS shared FROM itinerary i2 JOIN usertable u3 ON u3.id=i2.user_id JOIN itinerary_sharing s ON i2.id=s.itinerary_id JOIN usertable u2 ON u2.id=s.shared_to_id WHERE i2.archived != true AND s.active=true AND u2.id=$1 ORDER BY start DESC, finish DESC, title, id DESC OFFSET $2 LIMIT $3",
        user_id,
        offset,
        limit);
    std::vector<ItineraryPgDao::itinerary_summary> itineraries;
    for (auto r : result) {
      itinerary_summary it;
      it.id = r["id"].as<long>();
      std::string s;
      if (r["start"].to(s)) {
        DateTime start(s);
        it.start = start.time_tp();
      }
      if (r["finish"].to(s)) {
        DateTime finish(s);
        it.finish = finish.time_tp();
      }
      r["title"].to(it.title);
      if (!r["nickname"].is_null())
        it.owner_nickname = r["nickname"].as<std::string>();
      if (!r["shared"].is_null())
        it.shared = r["shared"].as<bool>();
      itineraries.push_back(it);
    }
    tx.commit();
    return itineraries;
  } catch (const std::exception &e) {
    std::cerr << "Exception executing query to fetch itineraries: "
              << e.what() << '\n';
    throw;
  }
}

std::optional<ItineraryPgDao::itinerary> ItineraryPgDao::get_itinerary_summary(
    std::string user_id, long itinerary_id)
{
  try {
    work tx(*connection);
    auto details = get_itinerary_details(tx, user_id, itinerary_id);
    if (details.has_value()) {
      ItineraryPgDao::itinerary it(details.value());
      // Query routes, tracks and waypoints
      auto route_result = tx.exec_params(
          "SELECT r.id, r.name, r.color AS color_key, "
          "rc.value AS color_description, "
          "distance, ascent, descent, lowest, highest "
          "FROM itinerary_route r "
          "LEFT JOIN path_color rc ON r.color=rc.key "
          "WHERE itinerary_id=$1 ORDER BY name, id",
          itinerary_id);
      for (const auto &rt : route_result) {
        path_summary route;
        if (!rt["id"].is_null())
          route.id = rt["id"].as<long>();
        if (!rt["name"].is_null())
          route.name = rt["name"].as<std::string>();
        if (!rt["color_key"].is_null())
          route.color_key = rt["color_key"].as<std::string>();
        if (!rt["color_description"].is_null())
          route.color_description = rt["color_description"].as<std::string>();
        if (!rt["distance"].is_null())
          route.distance = rt["distance"].as<double>();
        if (!rt["ascent"].is_null())
          route.ascent = rt["ascent"].as<double>();
        if (!rt["descent"].is_null())
          route.descent = rt["descent"].as<double>();
        if (!rt["lowest"].is_null())
          route.lowest = rt["lowest"].as<double>();
        if (!rt["highest"].is_null())
          route.highest = rt["highest"].as<double>();
        it.routes.push_back(route);
      }
      auto track_result = tx.exec_params(
          "SELECT t.id, t.name, t.color AS color_key, "
          "rc.value AS color_description, "
          "distance, ascent, descent, lowest, highest "
          "FROM itinerary_track t "
          "LEFT JOIN path_color rc ON t.color=rc.key "
          "WHERE itinerary_id=$1 ORDER BY name, id",
          itinerary_id);
      for (const auto &tk : track_result) {
        path_summary track;
        if (!tk["id"].is_null())
          track.id = tk["id"].as<long>();
        if (!tk["name"].is_null())
          track.name = tk["name"].as<std::string>();
        if (!tk["color_key"].is_null())
          track.color_key = tk["color_key"].as<std::string>();
        if (!tk["color_description"].is_null())
          track.color_description = tk["color_description"].as<std::string>();
        if (!tk["distance"].is_null())
          track.distance = tk["distance"].as<double>();
        if (!tk["ascent"].is_null())
          track.ascent = tk["ascent"].as<double>();
        if (!tk["descent"].is_null())
          track.descent = tk["descent"].as<double>();
        if (!tk["lowest"].is_null())
          track.lowest = tk["lowest"].as<double>();
        if (!tk["highest"].is_null())
          track.highest = tk["highest"].as<double>();
        it.tracks.push_back(track);
      }
      auto waypoint_result = tx.exec_params(
          "SELECT id, name, "
          "comment, type, "
          "symbol, ws.value AS symbol_text "
          "FROM itinerary_waypoint iw "
          "LEFT JOIN waypoint_symbol ws ON iw.symbol=ws.key "
          "WHERE itinerary_id=$1 ORDER BY name, symbol, id",
          itinerary_id);
      for (const auto &wpt : waypoint_result) {
        waypoint_summary waypoint;
        wpt["id"].to(waypoint.id);
        if (!wpt["name"].is_null())
          waypoint.name = wpt["name"].as<std::string>();
        if (!wpt["comment"].is_null())
          waypoint.comment = wpt["comment"].as<std::string>();
        if (!wpt["type"].is_null())
          waypoint.type = wpt["type"].as<std::string>();
        if (!wpt["symbol_text"].is_null())
          waypoint.symbol = wpt["symbol_text"].as<std::string>();
        if (!waypoint.symbol.has_value() && !wpt["symbol"].is_null())
          waypoint.symbol = wpt["symbol"].as<std::string>();
        it.waypoints.push_back(waypoint);
      }

      tx.commit();
      return it;
    } else {
      return std::optional<ItineraryPgDao::itinerary>();
    }
  } catch (const std::exception &e) {
    std::cerr << "Exception executing query to fetch itinerary details: "
              << e.what() << '\n';
    throw;
  }
}

std::optional<ItineraryPgDao::itinerary_complete>
    ItineraryPgDao::get_itinerary_complete(
        std::string user_id, long itinerary_id)
{
  try {
    work tx(*connection);
    auto itinerary_details =
      get_itinerary_details(tx, user_id, itinerary_id);
    ItineraryPgDao::itinerary_complete itinerary(itinerary_details.value());

    if (itinerary_details.has_value()) {

      // Don't leak details of whom another user may also be sharing an
      // itinerary with.
      if (has_user_itinerary_modification_access(tx, user_id, itinerary_id))
          itinerary.shares = get_itinerary_shares(tx, itinerary_id);

      itinerary.routes = get_routes(tx, user_id, itinerary_id);
      itinerary.waypoints = get_waypoints(tx, user_id, itinerary_id);
      itinerary.tracks = get_tracks(tx, user_id, itinerary_id);
    }

    return itinerary;

  } catch (const std::exception &e) {
    std::cerr
      << "Exception executing query to fetch complete itinerary details: "
      << e.what() << '\n';
    throw;
  }
}

std::string ItineraryPgDao::get_itinerary_title(
    std::string user_id, long itinerary_id)
{
  work tx(*connection);
  validate_user_itinerary_read_access(tx, user_id, itinerary_id);
  auto r = tx.exec_params1("SELECT title FROM itinerary WHERE id=$1",
                           itinerary_id);
  return r[0].as<std::string>();
}

std::optional<ItineraryPgDao::itinerary_description>
    ItineraryPgDao::get_itinerary_description(
        std::string user_id, long itinerary_id)
{
  ItineraryPgDao::itinerary_description it;
  try {
    work tx(*connection);
    auto r = tx.exec_params1(
        "SELECT id, start, finish, title, description "
        "FROM itinerary "
        "WHERE archived != true AND user_id=$1 AND id=$2"
        ,
        user_id,
        itinerary_id
      );
    it.id = r["id"].as<long>();
    std::string s;
    if (r["start"].to(s)) {
      DateTime start(s);
      it.start = start.time_tp();
    }
    if (r["finish"].to(s)) {
      DateTime finish(s);
      it.finish = finish.time_tp();
    }
    r["title"].to(it.title);
    if (!r["description"].is_null())
      it.description = r["description"].as<std::string>();
    tx.commit();
    return it;
  } catch (const pqxx::unexpected_rows) {
    return it;
  } catch (const std::exception &e) {
    std::cerr << "Exception executing query to fetch itinerary description: "
              << e.what() << '\n';
    throw;
  }
}

std::optional<ItineraryPgDao::itinerary_detail>
    ItineraryPgDao::get_itinerary_details(
        work& tx,
        std::string user_id, long itinerary_id)
{
  const std::string sql =
    "SELECT i.id, i.start, i.finish, i.title, i.description, "
    "u.nickname AS owned_by_nickname, null AS shared_to_nickname "
    "FROM itinerary i JOIN usertable u ON i.user_id=u.id "
    "WHERE i.archived != true AND i.id=$2 AND u.id=$1 "
    "UNION SELECT i2.id, i2.start, i2.finish, i2.title, i2.description, "
    "ownr.nickname AS owned_by_nickname, u2.nickname AS shared_to_nickname "
    "FROM itinerary i2 JOIN itinerary_sharing s ON i2.id=s.itinerary_id "
    "JOIN usertable u2 ON s.shared_to_id=u2.id "
    "JOIN usertable ownr ON ownr.id=i2.user_id "
    "WHERE i2.id=$2 AND i2.archived != true AND s.active=true AND u2.id=$1 "
    "ORDER BY shared_to_nickname DESC";

  // std::cout << "SQL:: " << sql << '\n';
  auto result = tx.exec_params(
      sql,
      user_id,
      itinerary_id
    );
  ItineraryPgDao::itinerary_detail it;
  if (result.empty()) {
    return it;
  }
  auto r = result[0];
  it.id = r["id"].as<long>();
  std::string s;
  if (r["start"].to(s)) {
    DateTime start(s);
    it.start = start.time_tp();
  }
  if (r["finish"].to(s)) {
    DateTime finish(s);
    it.finish = finish.time_tp();
  }
  r["title"].to(it.title);
  if (!r["owned_by_nickname"].is_null())
    it.owner_nickname = r["owned_by_nickname"].as<std::string>();
  if (!r["shared_to_nickname"].is_null())
    it.shared_to_nickname = r["shared_to_nickname"].as<std::string>();
  it.shared = it.shared_to_nickname.has_value();
  if (!r["description"].is_null())
    it.description = r["description"].as<std::string>();
  return it;
}

long ItineraryPgDao::save(
    std::string user_id,
    ItineraryPgDao::itinerary_description itinerary)
{
  try {
    work tx(*connection);
    std::optional<std::string> start;
    std::optional<std::string> finish;
    if (itinerary.start.has_value())
      start = DateTime(itinerary.start.value()).get_time_as_iso8601_gmt();
    if (itinerary.finish.has_value())
      finish = DateTime(itinerary.finish.value()).get_time_as_iso8601_gmt();
    long id = -1;
    if (itinerary.id.has_value()) {
      id = itinerary.id.value();
      tx.exec_params(
          "UPDATE itinerary "
          "SET title=$3, start=$4, finish=$5, description=$6 "
          "WHERE user_id=$1 AND id=$2",
          user_id,
          itinerary.id,
          itinerary.title,
          start,
          finish,
          itinerary.description
        );
    } else {
      auto r = tx.exec_params1(
          "INSERT INTO itinerary "
          "(user_id, title, start, finish, description) "
          "VALUES ($1, $2, $3, $4, $5) RETURNING id",
          user_id,
          itinerary.title,
          start,
          finish,
          itinerary.description
        );
      r["id"].to(id);
    }
    tx.commit();
    return id;
  } catch (const std::exception &e) {
    std::cerr << "Exception saving itinerary description: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::delete_itinerary(
    std::string user_id,
    long itinerary_id)
{
  work tx(*connection);
  tx.exec_params("DELETE FROM ITINERARY WHERE user_id=$1 AND id=$2",
                 user_id,
                 itinerary_id);
  tx.commit();
}

std::vector<ItineraryPgDao::route>
    ItineraryPgDao::get_routes(std::string user_id,
                               long itinerary_id,
                               const std::vector<long> &route_ids)
{
  std::vector<route> routes;
  if (route_ids.empty())
    return routes;
  work tx(*connection);
  const std::string route_ids_sql = dao_helper::to_sql_array(route_ids);
  auto result = tx.exec_params(
      "SELECT r.id AS route_id, r.name AS route_name, r.color AS path_color_key, "
      "rc.value AS path_color_value, "
      "rc.html_code, r.distance, r.ascent, r.descent, r.lowest, r.highest, "
      "p.id AS point_id, "
      "ST_X(p.geog::geometry) as lng, ST_Y(p.geog::geometry) as lat, "
      "p.name AS point_name, p.comment, p.description, p.symbol, p.altitude "
      "FROM itinerary_route r "
      "JOIN itinerary i ON i.id=r.itinerary_id "
      "LEFT JOIN path_color rc ON r.color=rc.key "
      "LEFT JOIN itinerary_route_point p ON r.id=p.itinerary_route_id "
      "LEFT JOIN itinerary_sharing sh "
      "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1 "
      "WHERE i.archived != true AND (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
      "AND r.itinerary_id = $2 AND r.id=ANY($3) "
      "ORDER BY r.id, p.id",
      user_id,
      itinerary_id,
      route_ids_sql
    );
  route rt;
  for (const auto &r : result) {
    long route_id = r["route_id"].as<long>();
    if (!rt.id.has_value() || rt.id.value() != route_id) {
      if (rt.id.has_value()) {
        routes.push_back(rt);
      }
      rt = route();
      rt.id = route_id;
      if (!r["route_name"].is_null())
        rt.name = r["route_name"].as<std::string>();
      if (!r["path_color_key"].is_null())
        rt.color_key = r["path_color_key"].as<std::string>();
      if (!r["path_color_value"].is_null())
        rt.color_description = r["path_color_value"].as<std::string>();
      if (!r["html_code"].is_null())
        rt.html_code = r["html_code"].as<std::string>();
      if (!r["distance"].is_null())
        rt.distance = r["distance"].as<double>();
      if (!r["ascent"].is_null())
        rt.ascent = r["ascent"].as<double>();
      if (!r["descent"].is_null())
        rt.descent = r["descent"].as<double>();
      if (!r["lowest"].is_null())
        rt.lowest = r["lowest"].as<double>();
      if (!r["highest"].is_null())
        rt.highest = r["highest"].as<double>();
    }
    if (!r["point_id"].is_null()) {
      route_point p;
      p.id = r["point_id"].as<long>();
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      if (!r["point_name"].is_null())
        p.name = r["point_name"].as<std::string>();
      if (!r["comment"].is_null())
        p.comment = r["comment"].as<std::string>();
      if (!r["description"].is_null())
        p.description = r["description"].as<std::string>();
      if (!r["symbol"].is_null())
        p.symbol = r["symbol"].as<std::string>();
      if (!r["altitude"].is_null())
        p.altitude = r["altitude"].as<double>();
      rt.points.push_back(p);
    }
  }
  if (rt.id.has_value())
    routes.push_back(rt);
  tx.commit();
  return routes;
}

std::vector<ItineraryPgDao::route>
    ItineraryPgDao::get_routes(pqxx::work &tx,
                               std::string user_id,
                               long itinerary_id)
{
  std::vector<route> routes;
  auto result = tx.exec_params(
      "SELECT r.id AS route_id, r.name AS route_name, r.color AS path_color_key, "
      "rc.value AS path_color_value, "
      "rc.html_code, r.distance, r.ascent, r.descent, r.lowest, r.highest, "
      "p.id AS point_id, "
      "ST_X(p.geog::geometry) as lng, ST_Y(p.geog::geometry) as lat, "
      "p.name AS point_name, p.comment, p.description, p.symbol, p.altitude "
      "FROM itinerary_route r "
      "JOIN itinerary i ON i.id=r.itinerary_id "
      "LEFT JOIN path_color rc ON r.color=rc.key "
      "LEFT JOIN itinerary_route_point p ON r.id=p.itinerary_route_id "
      "LEFT JOIN itinerary_sharing sh "
      "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1 "
      "WHERE i.archived != true AND (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
      "AND r.itinerary_id = $2 "
      "ORDER BY r.id, p.id",
      user_id,
      itinerary_id
    );
  route rt;
  for (const auto &r : result) {
    long route_id = r["route_id"].as<long>();
    if (!rt.id.has_value() || rt.id.value() != route_id) {
      if (rt.id.has_value()) {
        routes.push_back(rt);
      }
      rt = route();
      rt.id = route_id;
      if (!r["route_name"].is_null())
        rt.name = r["route_name"].as<std::string>();
      if (!r["path_color_key"].is_null())
        rt.color_key = r["path_color_key"].as<std::string>();
      if (!r["path_color_value"].is_null())
        rt.color_description = r["path_color_value"].as<std::string>();
      if (!r["html_code"].is_null())
        rt.html_code = r["html_code"].as<std::string>();
      if (!r["distance"].is_null())
        rt.distance = r["distance"].as<double>();
      if (!r["ascent"].is_null())
        rt.ascent = r["ascent"].as<double>();
      if (!r["descent"].is_null())
        rt.descent = r["descent"].as<double>();
      if (!r["lowest"].is_null())
        rt.lowest = r["lowest"].as<double>();
      if (!r["highest"].is_null())
        rt.highest = r["highest"].as<double>();
    }
    if (!r["point_id"].is_null()) {
      route_point p;
      p.id = r["point_id"].as<long>();
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      if (!r["point_name"].is_null())
        p.name = r["point_name"].as<std::string>();
      if (!r["comment"].is_null())
        p.comment = r["comment"].as<std::string>();
      if (!r["description"].is_null())
        p.description = r["description"].as<std::string>();
      if (!r["symbol"].is_null())
        p.symbol = r["symbol"].as<std::string>();
      if (!r["altitude"].is_null())
        p.altitude = r["altitude"].as<double>();
      rt.points.push_back(p);
    }
  }
  if (rt.id.has_value())
    routes.push_back(rt);
  return routes;
}

std::vector<ItineraryPgDao::route>
    ItineraryPgDao::get_routes(std::string user_id,
                               long itinerary_id)
{
  work tx(*connection);
  const auto retval = get_routes(tx, user_id, itinerary_id);
  tx.commit();
  return retval;
}

std::vector<ItineraryPgDao::track>
    ItineraryPgDao::get_tracks(std::string user_id,
                               long itinerary_id,
                               const std::vector<long> &ids)
{
  std::vector<track> tracks;
  if (ids.empty())
    return tracks;
  work tx(*connection);
  const std::string ids_sql = dao_helper::to_sql_array(ids);
  const std::string sql =
    "SELECT t.id AS track_id, t.name AS track_name, t.color AS path_color_key, "
    "rc.value AS path_color_value, "
    "rc.html_code, t.distance, t.ascent, t.descent, t.lowest, t.highest, "
    "ts.id AS segment_id, p.id AS point_id, "
    "ts.distance AS ts_distance, ts.ascent AS ts_ascent, "
    "ts.descent AS ts_descent, ts.lowest AS ts_lowest, "
    "ts.highest AS ts_highest, "
    "ST_X(p.geog::geometry) as lng, ST_Y(p.geog::geometry) as lat, "
    "p.time, p.hdop, p.altitude "
    "FROM itinerary_track t "
    "JOIN itinerary i ON i.id=t.itinerary_id "
    "LEFT JOIN path_color rc ON t.color=rc.key "
    "LEFT JOIN itinerary_track_segment ts ON t.id=ts.itinerary_track_id "
    "LEFT JOIN itinerary_track_point p ON ts.id=p.itinerary_track_segment_id "
    "LEFT JOIN itinerary_sharing sh "
    "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1 "
    "WHERE i.archived != true AND (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
    "AND t.itinerary_id=$2 AND t.id=ANY($3) ORDER BY t.id, ts.id, p.id";
  // std::cout << "SQL: \n" << sql << '\n' << "track ids: " << ids_sql << '\n';
  auto result = tx.exec_params(
      sql,
      user_id,
      itinerary_id,
      ids_sql
    );
  // std::cout << "Got " << result.size() << " track points for itinerary " << itinerary_id << "\n";
  track trk;
  track_segment trkseg;
  for (const auto &r : result) {
    long track_id = r["track_id"].as<long>();
    if (!trk.id.has_value() || trk.id.value() != track_id) {
      if (trk.id.has_value()) {
        if (trkseg.id.has_value()) {
          trk.segments.push_back(trkseg);
          trkseg = track_segment();
        }
        tracks.push_back(trk);
      }
      trk = track();
      trk.id = track_id;
      if (!r["track_name"].is_null())
        trk.name = r["track_name"].as<std::string>();
      if (!r["distance"].is_null())
        trk.distance = r["distance"].as<double>();
      if (!r["ascent"].is_null())
        trk.ascent = r["ascent"].as<double>();
      if (!r["descent"].is_null())
        trk.descent = r["descent"].as<double>();
      if (!r["lowest"].is_null())
        trk.lowest = r["lowest"].as<double>();
      if (!r["highest"].is_null())
        trk.highest = r["highest"].as<double>();
      if (!r["path_color_key"].is_null())
        trk.color_key = r["path_color_key"].as<std::string>();
      if (!r["path_color_value"].is_null())
        trk.color_description = r["path_color_value"].as<std::string>();
      if (!r["html_code"].is_null())
        trk.html_code = r["html_code"].as<std::string>();
    }
    if (!r["segment_id"].is_null()) {
      long segment_id = r["segment_id"].as<long>();
      if (!trkseg.id.has_value() || trkseg.id.value() != segment_id) {
        if (trkseg.id.has_value()) {
          trk.segments.push_back(trkseg);
          trkseg = track_segment();
        }
        trkseg.id = segment_id;
        if (!r["ts_distance"].is_null())
          trkseg.distance = r["ts_distance"].as<double>();
        if (!r["ts_ascent"].is_null())
          trkseg.ascent = r["ts_ascent"].as<double>();
        if (!r["ts_descent"].is_null())
          trkseg.descent = r["ts_descent"].as<double>();
        if (!r["ts_lowest"].is_null())
          trkseg.lowest = r["ts_lowest"].as<double>();
        if (!r["ts_highest"].is_null())
          trkseg.highest = r["ts_highest"].as<double>();
      }
      if (!r["point_id"].is_null()) {
        track_point p;
        p.id = r["point_id"].as<long>();
        r["lng"].to(p.longitude);
        r["lat"].to(p.latitude);
        std::string s;
        if (r["time"].to(s)) {
          DateTime time(s);
          p.time = time.time_tp();
        }
        if (!r["hdop"].is_null())
          p.hdop = r["hdop"].as<double>();
        if (!r["altitude"].is_null())
          p.altitude = r["altitude"].as<double>();
        trkseg.points.push_back(p);
      }
    }
  }
  if (trk.id.has_value()) {
    if (trkseg.id.has_value())
      trk.segments.push_back(trkseg);
    tracks.push_back(trk);
  }
  tx.commit();
  return tracks;
}

std::vector<ItineraryPgDao::track>
    ItineraryPgDao::get_tracks(pqxx::work &tx,
                               std::string user_id,
                               long itinerary_id)
{
  std::vector<track> tracks;
  const std::string sql =
    "SELECT t.id AS track_id, t.name AS track_name, t.color AS path_color_key, "
    "rc.value AS path_color_value, "
    "rc.html_code, t.distance, t.ascent, t.descent, t.lowest, t.highest, "
    "ts.id AS segment_id, p.id AS point_id, "
    "ts.distance AS ts_distance, ts.ascent AS ts_ascent, "
    "ts.descent AS ts_descent, ts.lowest AS ts_lowest, "
    "ts.highest AS ts_highest, "
    "ST_X(p.geog::geometry) as lng, ST_Y(p.geog::geometry) as lat, "
    "p.time, p.hdop, p.altitude "
    "FROM itinerary_track t "
    "JOIN itinerary i ON i.id=t.itinerary_id "
    "LEFT JOIN path_color rc ON t.color=rc.key "
    "LEFT JOIN itinerary_track_segment ts ON t.id=ts.itinerary_track_id "
    "LEFT JOIN itinerary_track_point p ON ts.id=p.itinerary_track_segment_id "
    "LEFT JOIN itinerary_sharing sh "
    "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1 "
    "WHERE i.archived != true AND (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
    "AND t.itinerary_id=$2 ORDER BY t.id, ts.id, p.id";
  // std::cout << "SQL: \n" << sql << '\n';
  auto result = tx.exec_params(
      sql,
      user_id,
      itinerary_id
    );
  // std::cout << "Got " << result.size() << " track points for itinerary " << itinerary_id << "\n";
  track trk;
  track_segment trkseg;
  for (const auto &r : result) {
    long track_id = r["track_id"].as<long>();
    if (!trk.id.has_value() || trk.id.value() != track_id) {
      if (trk.id.has_value()) {
        if (trkseg.id.has_value()) {
          trk.segments.push_back(trkseg);
          trkseg = track_segment();
        }
        tracks.push_back(trk);
      }
      trk = track();
      trk.id = track_id;
      if (!r["track_name"].is_null())
        trk.name = r["track_name"].as<std::string>();
      if (!r["distance"].is_null())
        trk.distance = r["distance"].as<double>();
      if (!r["ascent"].is_null())
        trk.ascent = r["ascent"].as<double>();
      if (!r["descent"].is_null())
        trk.descent = r["descent"].as<double>();
      if (!r["lowest"].is_null())
        trk.lowest = r["lowest"].as<double>();
      if (!r["highest"].is_null())
        trk.highest = r["highest"].as<double>();
      if (!r["path_color_key"].is_null())
        trk.color_key = r["path_color_key"].as<std::string>();
      if (!r["path_color_value"].is_null())
        trk.color_description = r["path_color_value"].as<std::string>();
      if (!r["html_code"].is_null())
        trk.html_code = r["html_code"].as<std::string>();
    }
    if (!r["segment_id"].is_null()) {
      long segment_id = r["segment_id"].as<long>();
      if (!trkseg.id.has_value() || trkseg.id.value() != segment_id) {
        if (trkseg.id.has_value()) {
          trk.segments.push_back(trkseg);
          trkseg = track_segment();
        }
        trkseg.id = segment_id;
        if (!r["ts_distance"].is_null())
          trkseg.distance = r["ts_distance"].as<double>();
        if (!r["ts_ascent"].is_null())
          trkseg.ascent = r["ts_ascent"].as<double>();
        if (!r["ts_descent"].is_null())
          trkseg.descent = r["ts_descent"].as<double>();
        if (!r["ts_lowest"].is_null())
          trkseg.lowest = r["ts_lowest"].as<double>();
        if (!r["ts_highest"].is_null())
          trkseg.highest = r["ts_highest"].as<double>();
        // std::cout << "Read segment ID: " << trkseg.id.second;
        // if (trkseg.distance.first)
        //   std::cout << " with distance " << trkseg.distance.second;
        // std::cout << '\n';
      }
      if (!r["point_id"].is_null()) {
        track_point p;
        p.id = r["point_id"].as<long>();
        r["lng"].to(p.longitude);
        r["lat"].to(p.latitude);
        std::string s;
        if (r["time"].to(s)) {
          DateTime time(s);
          p.time = time.time_tp();
        }
        if (!r["hdop"].is_null())
          p.hdop = r["hdop"].as<double>();
        if (!r["altitude"].is_null())
          p.altitude = r["altitude"].as<double>();
        trkseg.points.push_back(p);
      }
    }
  }
  if (trk.id.has_value()) {
    if (trkseg.id.has_value())
      trk.segments.push_back(trkseg);
    tracks.push_back(trk);
  }
  return tracks;
}

std::vector<ItineraryPgDao::track>
    ItineraryPgDao::get_tracks(std::string user_id,
                               long itinerary_id)
{
  work tx(*connection);
  const auto retval = get_tracks(tx, user_id, itinerary_id);
  tx.commit();
  return retval;
}

std::vector<ItineraryPgDao::waypoint>
    ItineraryPgDao::get_waypoints(std::string user_id,
                                  long itinerary_id,
                                  const std::vector<long> &ids)
{
  std::vector<waypoint> waypoints;
  if (ids.empty())
    return waypoints;
  work tx(*connection);
  const std::string ids_sql = dao_helper::to_sql_array(ids);
  auto result = tx.exec_params(
      "SELECT w.id, w.name, "
      "ST_X(geog::geometry) as lng, ST_Y(geog::geometry) as lat, "
      "w.time, w.altitude, w.symbol, w.comment, w.description, "
      "w.extended_attributes, w.type, w.avg_samples "
      "FROM itinerary_waypoint w "
      "JOIN itinerary i ON i.id=w.itinerary_id "
      "LEFT JOIN itinerary_sharing sh "
      "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1 "
      "WHERE i.archived != true AND (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
      "AND w.itinerary_id=$2 AND w.id=ANY($3) "
      "ORDER BY name, symbol, id",
      user_id,
      itinerary_id,
      ids_sql);
  for (const auto &r : result) {
    waypoint wpt;
    wpt.id = r["id"].as<long>();
    if (!r["name"].is_null())
      wpt.name = r["name"].as<std::string>();
    r["lng"].to(wpt.longitude);
    r["lat"].to(wpt.latitude);
    std::string s;
    if (r["time"].to(s)) {
      DateTime time(s);
      wpt.time = time.time_tp();
    }
    if (!r["altitude"].is_null())
      wpt.altitude = r["altitude"].as<double>();
    if (!r["symbol"].is_null())
      wpt.symbol = r["symbol"].as<std::string>();
    if (!r["comment"].is_null())
      wpt.comment = r["comment"].as<std::string>();
    if (!r["description"].is_null())
      wpt.description = r["description"].as<std::string>();
    if (!r["extended_attributes"].is_null())
      wpt.extended_attributes = r["extended_attributes"].as<std::string>();
    if (!r["type"].is_null())
      wpt.type = r["type"].as<std::string>();
    if (!r["avg_samples"].is_null())
      wpt.avg_samples = r["avg_samples"].as<long>();
    waypoints.push_back(wpt);
  }
  tx.commit();
  return waypoints;
}

std::vector<ItineraryPgDao::waypoint>
    ItineraryPgDao::get_waypoints(pqxx::work &tx,
                                  std::string user_id,
                                  long itinerary_id)
{
  std::vector<waypoint> waypoints;
  auto result = tx.exec_params(
      "SELECT w.id, w.name, "
      "ST_X(geog::geometry) as lng, ST_Y(geog::geometry) as lat, "
      "w.time, w.altitude, w.symbol, w.comment, w.description, "
      "w.extended_attributes, w.type, w.avg_samples "
      "FROM itinerary_waypoint w "
      "JOIN itinerary i ON i.id=w.itinerary_id "
      "LEFT JOIN itinerary_sharing sh "
      "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1 "
      "WHERE i.archived != true AND (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
      "AND w.itinerary_id=$2 "
      "ORDER BY name, symbol, id",
      user_id,
      itinerary_id);
  for (const auto &r : result) {
    waypoint wpt;
    wpt.id = r["id"].as<long>();
    if (!r["name"].is_null())
      wpt.name = r["name"].as<std::string>();
    r["lng"].to(wpt.longitude);
    r["lat"].to(wpt.latitude);
    std::string s;
    if (r["time"].to(s)) {
      DateTime time(s);
      wpt.time = time.time_tp();
    }
    if (!r["altitude"].is_null())
      wpt.altitude = r["altitude"].as<double>();
    if (!r["symbol"].is_null())
      wpt.symbol = r["symbol"].as<std::string>();
    if (!r["comment"].is_null())
      wpt.comment = r["comment"].as<std::string>();
    if (!r["description"].is_null())
      wpt.description = r["description"].as<std::string>();
    if (!r["extended_attributes"].is_null())
      wpt.extended_attributes = r["extended_attributes"].as<std::string>();
    if (!r["type"].is_null())
      wpt.type = r["type"].as<std::string>();
    if (!r["avg_samples"].is_null())
      wpt.avg_samples = r["avg_samples"].as<long>();
    waypoints.push_back(wpt);
  }
  return waypoints;
}

std::vector<ItineraryPgDao::waypoint>
    ItineraryPgDao::get_waypoints(std::string user_id,
                                  long itinerary_id)
{
  work tx(*connection);
  const auto retval = get_waypoints(tx, user_id, itinerary_id);
  tx.commit();
  return retval;
}

void ItineraryPgDao::create_waypoints(
    work &tx,
    long itinerary_id,
    const std::vector<waypoint> &waypoints)
{
  if (waypoints.empty())
    return;
  const std::string ps_name = "create-waypoints";
  const std::string sql =
    "INSERT INTO itinerary_waypoint ("
    "itinerary_id, name, geog, altitude, time, comment, description, symbol, "
    "extended_attributes, type, avg_samples) "
    "VALUES ($1, $2, ST_SetSRID(ST_POINT($3, $4),4326), $5, $6, $7, $8, $9, "
    "$10, $11, $12)";
  connection->prepare(
      ps_name,
      sql);
  for (const auto &w : waypoints) {
    std::optional<std::string> timestr;
    if (w.time.has_value())
      timestr = DateTime(w.time.value()).get_time_as_iso8601_gmt();
    tx.exec_prepared(
        ps_name,
        itinerary_id,
        w.name,
        w.longitude,
        w.latitude,
        w.altitude,
        timestr,
        w.comment,
        w.description,
        w.symbol,
        w.extended_attributes,
        w.type,
        w.avg_samples);
  }
}

void ItineraryPgDao::create_waypoints(
    std::string user_id,
    long itinerary_id,
    const std::vector<waypoint> &waypoints)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    create_waypoints(tx, itinerary_id, waypoints);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst creating waypoints: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::create_route_points(
    work &tx,
    route &route)
{
  if (route.points.empty())
    return;
  const std::string ps_name = "create-route-points";
  const std::string sql =
    "INSERT INTO itinerary_route_point "
    "(itinerary_route_id, geog, altitude, name, "
    "comment, description, symbol) "
    "VALUES ($1, ST_SetSRID(ST_POINT($2, $3),4326), $4, $5, $6, $7, $8) "
    "RETURNING id";
  for (auto &p : route.points) {
    auto r = tx.exec_params1(
        sql,
        route.id,
        p.longitude,
        p.latitude,
        p.altitude,
        p.name,
        p.comment,
        p.description,
        p.symbol);
    p.id = r["id"].as<long>();
  }
}

void ItineraryPgDao::create_routes(
    work &tx,
    long itinerary_id,
    std::vector<route> &routes)
{
  if (routes.empty())
    return;
  const std::string ps_name = "create-routes";
  const std::string sql =
    "INSERT INTO itinerary_route "
    "(itinerary_id, name, color, distance, ascent, descent, lowest, highest) "
    "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id";
  for (auto &r : routes) {
    auto result = tx.exec_params1(
        sql,
        itinerary_id,
        r.name,
        r.color_key,
        r.distance,
        r.ascent,
        r.descent,
        r.lowest,
        r.highest);
    r.id = result["id"].as<long>();
    create_route_points(tx, r);
  }
}

void ItineraryPgDao::create_routes(
    std::string user_id,
    long itinerary_id,
    std::vector<route> &routes)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    create_routes(tx, itinerary_id, routes);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst creating routes: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::create_track_points(
    work &tx,
    track_segment &segment)
{
  if (segment.points.empty())
    return;
  const std::string ps_name = "create-track-points";
  const std::string sql =
    "INSERT INTO itinerary_track_point "
    "(itinerary_track_segment_id, geog, time, hdop, altitude) "
    "VALUES ($1, ST_SetSRID(ST_POINT($2, $3),4326), $4, $5, $6) "
    "RETURNING id";
  // int count = 0;
  for (auto &p : segment.points) {
    // count++;
    std::optional<std::string> timestr;
    if (p.time.has_value())
      timestr = DateTime(p.time.value()).get_time_as_iso8601_gmt();
    auto r = tx.exec_params1(
        sql,
        segment.id,
        p.longitude,
        p.latitude,
        timestr,
        p.hdop,
        p.altitude);
    p.id = r["id"].as<long>();
  }
}

void ItineraryPgDao::create_track_segments(
    work &tx,
    track &track)
{
  if (track.segments.empty())
    return;
  const std::string ps_name = "create-track-segments";
  const std::string sql =
    "INSERT INTO itinerary_track_segment "
    "(itinerary_track_id, distance, ascent, descent, lowest, highest) "
    "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id";
  for (auto &seg : track.segments) {
    auto r = tx.exec_params1(
        sql,
        track.id,
        seg.distance,
        seg.ascent,
        seg.descent,
        seg.lowest,
        seg.highest);
    seg.id = r["id"].as<long>();
    create_track_points(tx, seg);
  }
}

void ItineraryPgDao::create_tracks(
    work &tx,
    long itinerary_id,
    std::vector<track> &tracks)
{
  if (tracks.empty())
    return;
  const std::string ps_name = "create-tracks";
  const std::string sql =
    "INSERT INTO itinerary_track "
    "(itinerary_id, name, color, distance, ascent, descent, lowest, highest) "
    "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id";
  for (auto &t : tracks) {
    auto r = tx.exec_params1(
        sql,
        itinerary_id,
        t.name,
        t.color_key,
        t.distance,
        t.ascent,
        t.descent,
        t.lowest,
        t.highest);
    t.id = r["id"].as<long>();
    create_track_segments(tx, t);
  }
}

void ItineraryPgDao::create_tracks(
    std::string user_id,
    long itinerary_id,
    std::vector<track> &tracks)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    create_tracks(tx, itinerary_id, tracks);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst creating tracks: "
              << e.what() << '\n';
    throw;
  }
}

/**
 * Saves the passed itinerary features (routes, tracks & waypoints).
 *
 * \param user_id the ID of the user to validate.
 * \param itinerary_id the ID of the itinerary to check.
 * \param features object containing the features to be saved.
 * \throws ItineraryPgDao::TripForbiddenException if the user is forbidden.
 */
void ItineraryPgDao::create_itinerary_features(
    std::string user_id,
    long itinerary_id,
    ItineraryPgDao::itinerary_features &features)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    create_waypoints(tx, itinerary_id, features.waypoints);
    create_routes(tx, itinerary_id, features.routes);
    create_tracks(tx, itinerary_id, features.tracks);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst saving itinerary features: "
              << e.what() << '\n';
    throw;
  }
}

/**
 * Validates that the specified user_id has write/update/delete access to the
 * specified itinerary.
 *
 * \param user_id the ID of the user to validate.
 * \param itinerary_id the ID of the itinerary to check.
 *
 * \return true if the user can update the itinerary
 */
bool ItineraryPgDao::has_user_itinerary_modification_access(pqxx::work &tx,
                                                            std::string user_id,
                                                            long itinerary_id)
{
  auto r = tx.exec_params1(
      "SELECT COUNT(*) FROM itinerary WHERE user_id=$1 AND id=$2",
      user_id,
      itinerary_id
    );
  return (r[0].as<long>() == 1);
}

bool ItineraryPgDao::has_user_itinerary_modification_access(std::string user_id,
                                                            long itinerary_id)
{
  work tx(*connection);
  const auto retval = has_user_itinerary_modification_access(tx, user_id, itinerary_id);
  tx.commit();
  return retval;
}

/**
 * Validates that the specified user_id has write/update/delete access to the
 * specified itinerary.
 *
 * \param existing transaction context.  This method does not commit the transaction.
 * \param user_id the ID of the user to validate.
 * \param itinerary_id the ID of the itinerary to check.
 *
 * \throws ItineraryPgDao::TripForbiddenException if the user is forbidden.
 */
void ItineraryPgDao::validate_user_itinerary_modification_access(work &tx,
                                                           std::string user_id,
                                                           long itinerary_id)
{
  auto r = tx.exec_params1(
      "SELECT COUNT(*) FROM itinerary WHERE user_id=$1 AND id=$2",
      user_id,
      itinerary_id
    );
  if (r[0].as<long>() != 1)
    throw TripForbiddenException();
}

/**
 * Validates that the specified user_id has write/update/delete access to the
 * specified itinerary.
 *
 * \param user_id the ID of the user to validate.
 * \param itinerary_id the ID of the itinerary to check.
 *
 * \throws ItineraryPgDao::TripForbiddenException if the user is forbidden.
 */
void ItineraryPgDao::validate_user_itinerary_modification_access(std::string user_id,
                                                           long itinerary_id)
{
  work tx(*connection);
  validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
  tx.commit();
}

/**
 * Validates that the specified user_id has read access to the specified
 * itinerary.
 *
 * \param existing transaction context.  This method does not commit the transaction.
 * \param user_id the ID of the user to validate.
 * \param itinerary_id the ID of the itinerary to check.
 *
 * \throws ItineraryPgDao::TripForbiddenException if the user is forbidden.
 */
void ItineraryPgDao::validate_user_itinerary_read_access(work &tx,
                                                           std::string user_id,
                                                           long itinerary_id)
{
  auto r = tx.exec_params1(
      "SELECT sum(count) FROM ("
      "SELECT COUNT(*) FROM itinerary i "
      "WHERE i.archived != true AND user_id=$1 AND id=$2 "
      "UNION ALL "
      "SELECT COUNT (*) FROM itinerary_sharing "
      "WHERE active=true AND itinerary_id=$2 AND shared_to_id=$1) as q",
      user_id,
      itinerary_id
    );
  // If the user sets themselves as a shared user, the count is 2
  if (r[0].as<long>() < 1)
    throw TripForbiddenException();
}

/**
 * Validates that the specified user_id has read access to the specified
 * itinerary.
 *
 * \param user_id the ID of the user to validate.
 * \param itinerary_id the ID of the itinerary to check.
 *
 * \throws ItineraryPgDao::TripForbiddenException if the user is forbidden.
 */
void ItineraryPgDao::validate_user_itinerary_read_access(std::string user_id,
                                                         long itinerary_id)
{
  work tx(*connection);
  validate_user_itinerary_read_access(tx, user_id, itinerary_id);
  tx.commit();
}

void ItineraryPgDao::delete_features(
    std::string user_id,
    long itinerary_id,
    const selected_feature_ids &features)
{
  if (features.routes.empty() &&
      features.tracks.empty() &&
      features.waypoints.empty())
    return;

  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    if (!features.routes.empty()) {
      const std::string route_ids = dao_helper::to_sql_array(features.routes);
      tx.exec_params0(
          "DELETE FROM itinerary_route WHERE itinerary_id=$1 AND id=ANY($2)",
          itinerary_id,
          route_ids
        );
    }
    if (!features.tracks.empty()) {
      const std::string track_ids = dao_helper::to_sql_array(features.tracks);
      tx.exec_params0(
          "DELETE FROM itinerary_track WHERE itinerary_id=$1 AND id=ANY($2)",
          itinerary_id,
          track_ids
        );
    }
    if (!features.waypoints.empty()) {
      const std::string waypoint_ids =
        dao_helper::to_sql_array(features.waypoints);
      tx.exec_params0(
          "DELETE FROM itinerary_waypoint WHERE itinerary_id=$1 AND id=ANY($2)",
          itinerary_id,
          waypoint_ids
        );
    }
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst deleting itinerary features: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<std::pair<std::string, std::string>>
    ItineraryPgDao::get_georef_formats()
{
  std::vector<std::pair<std::string, std::string>> retval;
  work tx(*connection);
  try {
    result r = tx.exec("SELECT key, value FROM georef_format ORDER BY ord");
    for (auto i : r) {
      std::string key;
      i["key"].to(key);
      std::string value;
      i["value"].to(value);
      retval.push_back(std::make_pair(key, value));
    }
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst fetching list of georef_format: "
              << e.what() << '\n';
    throw;
  }
  return retval;
}

std::vector<std::pair<std::string, std::string>>
    ItineraryPgDao::get_waypoint_symbols()
{
  std::vector<std::pair<std::string, std::string>> retval;
  work tx(*connection);
  try {
    result r = tx.exec("SELECT key, value FROM waypoint_symbol ORDER BY value");
    for (auto i : r) {
      std::string key = i["key"].as<std::string>();
      std::string value = i["value"].as<std::string>();
      retval.push_back(std::make_pair(key, value));
    }
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst fetching list of waypoint_symbol: "
              << e.what() << '\n';
    throw;
  }
  return retval;
}

/**
 * \return a detailed waypoint object containing all the flat attributes.
 */
ItineraryPgDao::waypoint ItineraryPgDao::get_waypoint(
    std::string user_id,
    long itinerary_id,
    long waypoint_id)
{
  work tx(*connection);
  validate_user_itinerary_read_access(tx, user_id, itinerary_id);
  try {
    const std::string sql =
      "SELECT id, name, "
      "ST_X(geog::geometry) as lng, ST_Y(geog::geometry) as lat, altitude, "
      "time, symbol, comment, description, extended_attributes, type, "
      "avg_samples "
      "FROM itinerary_waypoint WHERE itinerary_id=$1 AND id=$2";
    row r = tx.exec_params1(sql,
                               itinerary_id,
                               waypoint_id);
    waypoint wpt;
    wpt.id = r["id"].as<long>();
    if (!r["name"].is_null())
      wpt.name = r["name"].as<std::string>();
    r["lng"].to(wpt.longitude);
    r["lat"].to(wpt.latitude);
    std::string s;
    if (r["time"].to(s)) {
      DateTime time(s);
      wpt.time = time.time_tp();
    }
    if (!r["altitude"].is_null())
      wpt.altitude = r["altitude"].as<double>();
    if (!r["symbol"].is_null())
      wpt.symbol = r["symbol"].as<std::string>();
    if (!r["comment"].is_null())
      wpt.comment = r["comment"].as<std::string>();
    if (!r["description"].is_null())
      wpt.description = r["description"].as<std::string>();
    if (!r["extended_attributes"].is_null())
      wpt.extended_attributes = r["extended_attributes"].as<std::string>();
    if (!r["type"].is_null())
      wpt.type = r["type"].as<std::string>();
    if (!r["avg_samples"].is_null())
      wpt.avg_samples = r["avg_samples"].as<long>();
    tx.commit();
    return wpt;
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst fetching waypoint: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::save(std::string user_id,
                          long itinerary_id,
                          waypoint &wpt)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);

    std::optional<std::string> time_str;
    if (wpt.time.has_value())
      time_str = DateTime(wpt.time.value()).get_time_as_iso8601_gmt();

    if (wpt.id.has_value()) {
      tx.exec_params(
          "UPDATE itinerary_waypoint SET name=$3, "
          "geog=ST_SetSRID(ST_POINT($4, $5),4326), altitude=$6, time=$7, "
          "symbol=$8, comment=$9, description=$10, avg_samples=$11, type=$12, "
          "extended_attributes=$13 WHERE itinerary_id=$1 AND id=$2",
          itinerary_id,
          wpt.id,
          wpt.name,
          wpt.longitude,
          wpt.latitude,
          wpt.altitude,
          time_str,
          wpt.symbol,
          wpt.comment,
          wpt.description,
          wpt.avg_samples,
          wpt.type,
          wpt.extended_attributes);
    } else {
      auto r = tx.exec_params1(
          "INSERT INTO itinerary_waypoint (itinerary_id, name, geog, altitude, "
          "time, symbol, comment, description, avg_samples, type, extended_attributes) "
          "VALUES ($1, $2, ST_SetSRID(ST_POINT($3, $4),4326), $5, $6, $7, $8, "
          "$9, $10, $11, $12) RETURNING id",
          itinerary_id,
          wpt.name,
          wpt.longitude,
          wpt.latitude,
          wpt.altitude,
          time_str,
          wpt.symbol,
          wpt.comment,
          wpt.description,
          wpt.avg_samples,
          wpt.type,
          wpt.extended_attributes);
      wpt.id = r["id"].as<long>();
    }
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst saving waypoint: "
              << e.what() << '\n';
    throw;
  }
}

/**
 * Saves the route.  If the route ID is set, all it's points are deleted and
 * re-created after updating the route itself.
 *
 * Otherwise a new route is created and it's points saved.
 */
void ItineraryPgDao::save(std::string user_id,
                          long itinerary_id,
                          route &route)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    if (route.id.has_value()) {
      // Validate the route belongs to this user
      auto check = tx.exec_params1(
          "SELECT COUNT(*) FROM itinerary_route "
          "WHERE itinerary_id=$1 AND id=$2",
          itinerary_id,
          route.id.value()
        );
      if (check[0].as<long>() != 1)
        throw TripForbiddenException();

      tx.exec_params("DELETE FROM itinerary_route_point "
                     "WHERE itinerary_route_id=$1",
                     route.id.value());
      auto r = tx.exec_params(
          "UPDATE itinerary_route "
          "SET name=$2, distance=$3, ascent=$4, descent=$5, lowest=$6, "
          "highest=$7, color=$8 "
          "WHERE id=$9 AND itinerary_id=$1",
          itinerary_id,
          route.name,
          route.distance,
          route.ascent,
          route.descent,
          route.lowest,
          route.highest,
          route.color_key,
          route.id
        );
    } else {
      auto r = tx.exec_params1(
          "INSERT INTO itinerary_route "
          "(itinerary_id, name, distance, ascent, descent, lowest, highest, "
          "color) "
          "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id",
          itinerary_id,
          route.name,
          route.distance,
          route.ascent,
          route.descent,
          route.lowest,
          route.highest,
          route.color_key
        );
      route.id = r["id"].as<long>();
    }
    create_route_points(tx, route);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst saving route: "
              << e.what() << '\n';
    throw;
  }
}

/**
 * Saves the track.  If the track ID is set, all it's segments and points are
 * deleted and re-created after updating the route itself.
 *
 * Otherwise, a new track is created and it's segments and points saved.
 */
void ItineraryPgDao::save(std::string user_id,
                          long itinerary_id,
                          track &track)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    if (track.id.has_value()) {
      // Validate the track belongs to this user
      auto check = tx.exec_params1(
          "SELECT COUNT(*) FROM itinerary_track "
          "WHERE itinerary_id=$1 AND id=$2",
          itinerary_id,
          track.id.value()
        );
      if (check[0].as<long>() != 1)
        throw TripForbiddenException();

      tx.exec_params("DELETE FROM itinerary_track_segment "
                     "WHERE itinerary_track_id=$1",
                     track.id.value());
      tx.exec_params(
          "UPDATE itinerary_track "
          "SET name=$3, color=$4, distance=$5, ascent=$6, descent=$7, "
          "lowest=$8, highest=$9 "
          "WHERE id=$2 AND itinerary_id=$1",
          itinerary_id,
          track.id,
          track.name,
          track.color_key,
          track.distance,
          track.ascent,
          track.descent,
          track.lowest,
          track.highest
        );
    } else {
      auto r = tx.exec_params1(
          "INSERT INTO itinerary_track "
          "(itinerary_id, name, color, distance, ascent, descent, lowest, highest) "
          "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id",
          itinerary_id,
          track.name,
          track.color_key,
          track.distance,
          track.ascent,
          track.descent,
          track.lowest,
          track.highest
        );
      track.id = r["id"].as<long>();
    }
    create_track_segments(tx, track);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst saving track: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::auto_color_paths(std::string user_id,
                                      long itinerary_id,
                                      const selected_feature_ids &selected)
{
  std::vector<std::string> colors;
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    connection->prepare(
        "auto-color-track",
        "UPDATE itinerary_track SET color=$3 WHERE itinerary_id=$1 AND id=$2"
      );
    connection->prepare(
        "auto-color-route",
        "UPDATE itinerary_route SET color=$3 WHERE itinerary_id=$1 AND id=$2"
      );
    auto color_result = tx.exec_params("SELECT key FROM path_color ORDER BY key");
    for (const auto &r : color_result)
      colors.push_back(r[0].as<std::string>());
    if (colors.empty())
      return;

    // Fetch the routes ordered by name so that we assign the colors in name order
    std::vector<long> sorted_routes;
    auto route_result = tx.exec_params(
        "SELECT id FROM itinerary_route r "
        "WHERE itinerary_id=$1 AND r.id=ANY($2) "
        "ORDER BY name, id",
        itinerary_id,
        dao_helper::to_sql_array(selected.routes));
    for (const auto &r : route_result)
      sorted_routes.push_back(r[0].as<long>());

    // Fetch the tracks ordered by name so that we assign the colors in name order
    std::vector<long> sorted_tracks;
    auto track_result = tx.exec_params(
        "SELECT id FROM itinerary_track r "
        "WHERE itinerary_id=$1 AND r.id=ANY($2) "
        "ORDER BY name, id",
        itinerary_id,
        dao_helper::to_sql_array(selected.tracks));
    for (const auto &r : track_result)
      sorted_tracks.push_back(r[0].as<long>());

    auto color = colors.begin();
    for (const auto &id : sorted_routes) {
      tx.exec_prepared(
          "auto-color-route",
          itinerary_id,
          id,
          *color);
      color++;
      if (color == colors.end())
        color = colors.begin();
    }
    for (const auto &id : sorted_tracks) {
      tx.exec_prepared(
          "auto-color-track",
          itinerary_id,
          id,
          *color);
      color++;
      if (color == colors.end())
        color = colors.begin();
    }
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst auto-assigning colors to paths: "
              << e.what() << '\n';
    throw;
  }
}

long ItineraryPgDao::get_itinerary_shares_count(std::string user_id,
                                               long itinerary_id)
{
  work tx(*connection);
  try {
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    auto r = tx.exec_params1(
        "SELECT COUNT(*) FROM itinerary_sharing "
        "WHERE itinerary_id=$1",
        itinerary_id);
    tx.commit();
    return r[0].as<long>();
  } catch (const std::exception &e) {
    std::cerr << "Exception fetching itinerary share count: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::itinerary_share>
    ItineraryPgDao::get_itinerary_shares(pqxx::work &tx,
                                         long itinerary_id,
                                         std::uint32_t offset,
                                         int limit)
{
  try {
    std::optional<int> fetch_limit;
    if (limit > 0)
      fetch_limit = limit;
    auto result = tx.exec_params(
        "SELECT sh.shared_to_id, u.nickname, sh.active "
        "FROM itinerary_sharing sh "
        "JOIN usertable u ON u.id=sh.shared_to_id "
        "WHERE sh.itinerary_id=$1 "
        "ORDER BY u.nickname "
        "OFFSET $2 LIMIT $3",
        itinerary_id,
        offset,
        fetch_limit);
    std::vector<itinerary_share> shares;
    for (const auto &r : result) {
      itinerary_share share;
      share.shared_to_id = r["shared_to_id"].as<long>();
      share.nickname = r["nickname"].as<std::string>();
      if (!r["active"].is_null())
        share.active = r["active"].as<bool>();
      shares.push_back(share);
    }
    return shares;
  } catch (const std::exception &e) {
    std::cerr << "Exception fetching itinerary shares: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::itinerary_share>
    ItineraryPgDao::get_itinerary_shares(std::string user_id,
                                         long itinerary_id,
                                         std::uint32_t offset,
                                         int limit)
{
  work tx(*connection);
  validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
  auto shares = get_itinerary_shares(tx, itinerary_id, offset, limit);
  tx.commit();
  return shares;
}

ItineraryPgDao::itinerary_share ItineraryPgDao::get_itinerary_share(
    std::string user_id,
    long itinerary_id,
    long shared_to_id)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    auto r = tx.exec_params1(
        "SELECT sh.shared_to_id, u.nickname, sh.active "
        "FROM itinerary_sharing sh "
        "JOIN usertable u ON u.id=sh.shared_to_id "
        "WHERE sh.itinerary_id=$1 AND sh.shared_to_id=$2"
        "ORDER BY u.nickname",
        itinerary_id,
        shared_to_id);
    itinerary_share share;
    share.shared_to_id = r["shared_to_id"].as<long>();
    share.nickname = r["nickname"].as<std::string>();
    if (!r["active"].is_null())
      share.active = r["active"].as<bool>();
    tx.commit();
    return share;
  } catch (const std::exception &e) {
    std::cerr << "Exception fetching itinerary share: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::save(std::string user_id,
                          long itinerary_id,
                          itinerary_share &share)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    tx.exec_params(
        "INSERT INTO itinerary_sharing (itinerary_id, shared_to_id, active) "
        "VALUES ($1, $2, $3) "
        "ON CONFLICT (itinerary_id, shared_to_id) DO UPDATE "
        "SET active=$3",
        itinerary_id,
        share.shared_to_id,
        share.active
      );
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception saving itinerary share: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::save(std::string user_id,
                          long itinerary_id,
                          std::vector<itinerary_share> &shares)
{
  work tx(*connection);
  try {
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    connection->prepare(
        "save-itinerary-shares",
        "INSERT INTO itinerary_sharing (itinerary_id, shared_to_id, active) "
        "VALUES ($1, $2, $3) "
        "ON CONFLICT (itinerary_id, shared_to_id) DO UPDATE "
        "SET active=$3");
    for (const auto &share : shares) {
      tx.exec_prepared(
          "save-itinerary-shares",
          itinerary_id,
          share.shared_to_id,
          share.active
        );
    }
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception saving itinerary shares: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::activate_itinerary_shares(
    std::string user_id,
    long itinerary_id,
    const std::vector<long> &shared_to_ids,
    bool activate)
{
  try {
    work tx(*connection);
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    const std::string ids_sql = dao_helper::to_sql_array(shared_to_ids);
    tx.exec_params(
        "UPDATE itinerary_sharing SET active=$3 "
        "WHERE itinerary_id=$1 AND shared_to_id=ANY($2)",
        itinerary_id,
        ids_sql,
        activate);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception activating itinerary shares: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::delete_itinerary_shares(
    std::string user_id,
    long itinerary_id,
    const std::vector<long> &shared_to_ids)
{
  try {
    work tx(*connection);
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    const std::string ids_sql = dao_helper::to_sql_array(shared_to_ids);
    tx.exec_params(
        "DELETE FROM itinerary_sharing "
        "WHERE itinerary_id=$1 AND shared_to_id=ANY($2)",
        itinerary_id,
        ids_sql);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception activating itinerary shares: "
              << e.what() << '\n';
    throw;
  }
}

YAML::Node ItineraryPgDao::track_point::encode(
    const ItineraryPgDao::track_point& rhs)
{
  auto node = location::encode(rhs);
  if (rhs.time.has_value()) {
    DateTime dt(rhs.time.value());
    node["time"] = dt.get_time_as_iso8601_gmt();
  } else {
    node["time"] = YAML::Null;
  }
  if (rhs.hdop.has_value())
    node["hdop"] = rhs.hdop.value();
  else
    node["hdop"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::track_point::decode(
    const YAML::Node& node, ItineraryPgDao::track_point& rhs)
{
  if (node["time"] && !node["time"].IsNull()) {
    DateTime time(node["time"].as<std::string>());
    rhs.time = time.time_tp();
  }
  if (node["hdop"] && !node["hdop"].IsNull())
    rhs.hdop = node["hdop"].as<float>();
  return location::decode(node, rhs);
}

YAML::Node ItineraryPgDao::track_segment::encode(
    const ItineraryPgDao::track_segment& rhs)
{
  auto node = ItineraryPgDao::path_base::encode(rhs);
  node["points"] = rhs.points;
  return node;
}

bool ItineraryPgDao::track_segment::decode(
    const YAML::Node& node, ItineraryPgDao::track_segment& rhs)
{
  rhs.points = node["points"].as<std::vector<track_point>>();
  return ItineraryPgDao::path_base::decode(node, rhs);
}

YAML::Node ItineraryPgDao::track::encode(const ItineraryPgDao::track& rhs)
{
  auto node = ItineraryPgDao::path_summary::encode(rhs);;
  node["segments"] = rhs.segments;
  return node;
}

bool ItineraryPgDao::track::decode(
    const YAML::Node& node, ItineraryPgDao::track& rhs)
{
  rhs.segments =
    node["segments"].as<std::vector<ItineraryPgDao::track_segment>>();
  return ItineraryPgDao::path_summary::decode(node, rhs);
}

YAML::Node ItineraryPgDao::waypoint_base::encode(
    const ItineraryPgDao::waypoint_base& rhs)
{
  YAML::Node node;
  if (rhs.name.has_value())
    node["name"] = rhs.name.value();
  else
    node["name"] = YAML::Null;
  if (rhs.comment.has_value())
    node["comment"] = rhs.comment.value();
  else
    node["comment"] = YAML::Null;
  if (rhs.symbol.has_value())
    node["symbol"] = rhs.symbol.value();
  else
    node["symbol"] = YAML::Null;
  if (rhs.type.has_value())
    node["type"] = rhs.type.value();
  else
    node["type"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::waypoint_base::decode(
    const YAML::Node& node, ItineraryPgDao::waypoint_base& rhs)
{
  if (node["name"] && !node["name"].IsNull())
    rhs.name = node["name"].as<std::string>();
  if (node["comment"] && !node["comment"].IsNull())
    rhs.comment = node["comment"].as<std::string>();
  if (node["symbol"] && !node["symbol"].IsNull())
    rhs.symbol = node["symbol"].as<std::string>();
  if (node["type"] && !node["type"].IsNull())
    rhs.type = node["type"].as<std::string>();
  return true;
}

YAML::Node ItineraryPgDao::waypoint::encode(const ItineraryPgDao::waypoint& rhs)
{
  auto node = location::encode(rhs);
  auto base = ItineraryPgDao::waypoint_base::encode(rhs);
  // switch (base.Type()) {
  //   case YAML::NodeType::Scalar:
  //     std::cout << "scalar\n";
  //     break;
  //   case YAML::NodeType::Sequence:
  //     std::cout << "sequence\n";
  //     break;
  //   case YAML::NodeType::Map:
  //     std::cout << "map\n";
  //     break;
  //   default:
  //     std::cout << "unknown\n";
  // }
  if (base.IsMap())
    for (const auto& b : base)
      node[b.first] = b.second;

  if (rhs.time.has_value()) {
    DateTime dt(rhs.time.value());
    node["time"] = dt.get_time_as_iso8601_gmt();
  } else {
    node["time"] = YAML::Null;
  }
  if (rhs.description.has_value())
    node["description"] = rhs.description.value();
  else
    node["description"] = YAML::Null;
  if (rhs.extended_attributes.has_value())
    node["extended_attributes"] = rhs.extended_attributes.value();
  else
    node["extended_attributes"] = YAML::Null;
  if (rhs.type.has_value())
    node["type"] = rhs.type.value();
  else
    node["type"] = YAML::Null;
  if (rhs.avg_samples.has_value())
    node["samples"] = rhs.avg_samples.value();
  else
    node["samples"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::waypoint::decode(
    const YAML::Node& node, ItineraryPgDao::waypoint& rhs)
{
  if (node["time"] && !node["time"].IsNull()) {
    DateTime time(node["time"].as<std::string>());
    rhs.time = time.time_tp();
  }
  if (node["description"] && !node["description"].IsNull())
    rhs.description = node["description"].as<std::string>();
  if (node["extended_attributes"] && !node["extended_attributes"].IsNull())
    rhs.extended_attributes = node["extended_attributes"].as<std::string>();
  if (node["type"] && !node["type"].IsNull())
    rhs.type = node["type"].as<std::string>();
  if (node["samples"] && !node["samples"].IsNull())
    rhs.avg_samples = node["samples"].as<long>();
  auto success = ItineraryPgDao::waypoint_base::decode(node, rhs);
  return location::decode(node, rhs) && success;
}

YAML::Node ItineraryPgDao::itinerary_share::encode(
    const ItineraryPgDao::itinerary_share& rhs)
{
  YAML::Node node;
  node["nickname"] = rhs.nickname;
  if (rhs.active.has_value())
    node["active"] = rhs.active.value();
  else
    node["active"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::itinerary_share::decode(
    const YAML::Node& node, ItineraryPgDao::itinerary_share& rhs)
{
  rhs.nickname = node["nickname"].as<std::string>();
  if (node["active"] && !node["active"].IsNull()) {
    rhs.active = node["active"].as<bool>();
  }
  return true;
}

YAML::Node ItineraryPgDao::itinerary_base::encode(
    const ItineraryPgDao::itinerary_base& rhs)
{
  YAML::Node node;
  if (rhs.id.has_value())
    node["id"] = rhs.id.value();
  else
    node["id"] = YAML::Null;
  if (rhs.start.has_value()) {
    DateTime start(rhs.start.value());
    node["start"] = start.get_time_as_iso8601_gmt();
  } else {
    node["start"] = YAML::Null;
  }
  if (rhs.finish.has_value()) {
    DateTime finish(rhs.finish.value());
    node["finish"] = finish.get_time_as_iso8601_gmt();
  } else {
    node["finish"] = YAML::Null;
  }
  node["title"] = rhs.title;
  return node;
}

bool ItineraryPgDao::itinerary_base::decode(
    const YAML::Node& node, ItineraryPgDao::itinerary_base& rhs)
{
  if (node["id"] && !node["id"].IsNull())
    rhs.id = node["id"].as<long>();
  if (node["start"] && !node["start"].IsNull()) {
    DateTime start(node["start"].as<std::string>());
    rhs.start = start.time_tp();
  }
  if (node["finish"] && !node["finish"].IsNull()) {
    DateTime finish(node["finish"].as<std::string>());
    rhs.finish.value() = finish.time_tp();
  }
  rhs.title = node["title"].as<std::string>();
  return true;
}

YAML::Node ItineraryPgDao::itinerary_summary::encode(
    const ItineraryPgDao::itinerary_summary& rhs)
{
  auto node = ItineraryPgDao::itinerary_base::encode(rhs);
  if (rhs.owner_nickname.has_value())
    node["owned_by_nickname"] = rhs.owner_nickname.value();
  else
    node["owned_by_nickname"] = YAML::Null;
  if (rhs.shared.has_value())
    node["shared"] = rhs.shared.value();
  else
    node["shared"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::itinerary_summary::decode(
    const YAML::Node& node, ItineraryPgDao::itinerary_summary& rhs)
{
  if (node["owned_by_nickname"] && !node["owned_by_nickname"].IsNull())
    rhs.owner_nickname = node["owned_by_nickname"].as<std::string>();
  if (node["shared"] && !node["shared"].IsNull())
    rhs.shared = node["shared"].as<bool>();
  return ItineraryPgDao::itinerary_base::decode(node, rhs);
}

YAML::Node ItineraryPgDao::itinerary_description::encode(
    const ItineraryPgDao::itinerary_description& rhs)
{
  auto node = ItineraryPgDao::itinerary_summary::encode(rhs);
  if (rhs.description.has_value())
    node["description"] = rhs.description.value();
  else
    node["description"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::itinerary_description::decode(
    const YAML::Node& node, ItineraryPgDao::itinerary_description& rhs)
{
  if (node["description"] && !node["description"].IsNull())
    rhs.description = node["description"].as<std::string>();
  return ItineraryPgDao::itinerary_summary::decode(node, rhs);
}

YAML::Node ItineraryPgDao::itinerary_detail::encode(
    const ItineraryPgDao::itinerary_detail& rhs)
{
  auto node = ItineraryPgDao::itinerary_description::encode(rhs);
  if (rhs.shared_to_nickname.has_value())
    node["shared_to_nickname"] = rhs.shared_to_nickname.value();
  else
    node["shared_to_nickname"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::itinerary_detail::decode(
    const YAML::Node& node, ItineraryPgDao::itinerary_detail& rhs)
{
  if (node["shared_to_nickname"] && !node["shared_to_nickname"].IsNull())
    rhs.shared_to_nickname = node["shard_to_nickname"].as<std::string>();
  return ItineraryPgDao::itinerary_description::decode(node, rhs);
}

YAML::Node
    ItineraryPgDao::itinerary_complete::encode(const itinerary_complete& rhs)
{
  auto node = ItineraryPgDao::itinerary_detail::encode(rhs);
  node["itinerary_shares"] = rhs.shares;
  node["routes"] = rhs.routes;
  node["waypoints"] = rhs.waypoints;
  node["tracks"] = rhs.tracks;
  return node;
}

bool ItineraryPgDao::itinerary_complete::decode(
    const YAML::Node& node, itinerary_complete& rhs)
{
  rhs.shares =
    node["itinerary_shares"].as<std::vector<ItineraryPgDao::itinerary_share>>();
  rhs.routes = node["routes"].as<std::vector<ItineraryPgDao::route>>();
  rhs.waypoints =
    node["waypoints"].as<std::vector<ItineraryPgDao::waypoint>>();
  rhs.tracks = node["tracks"].as<std::vector<ItineraryPgDao::track>>();
  return ItineraryPgDao::itinerary_description::decode(node, rhs);
}

YAML::Node
    ItineraryPgDao::route_point::encode(const ItineraryPgDao::route_point& rhs)
{
  YAML::Node node = location::encode(rhs);
  if (rhs.name.has_value())
    node["name"] = rhs.name.value();
  else
    node["name"] = YAML::Null;
  if (rhs.comment.has_value())
    node["comment"] = rhs.comment.value();
  else
    node["comment"] = YAML::Null;
  if (rhs.description.has_value())
    node["description"] = rhs.description.value();
  else
    node["description"] = YAML::Null;
  if (rhs.symbol.has_value())
    node["symbol"] = rhs.symbol.value();
  else
    node["symbol"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::route_point::decode(
    const YAML::Node& node, ItineraryPgDao::route_point& rhs)
{
  if (node["name"] && !node["name"].IsNull())
    rhs.name = node["name"].as<std::string>();
  if (node["comment"] && !node["comment"].IsNull())
    rhs.comment = node["comment"].as<std::string>();
  if (node["description"] && !node["description"].IsNull())
    rhs.description = node["description"].as<std::string>();
  if (node["symbol"] && !node["symbol"].IsNull())
    rhs.symbol = node["symbol"].as<std::string>();
  return location::decode(node, rhs);
}

YAML::Node
    ItineraryPgDao::path_base::encode(const ItineraryPgDao::path_base& rhs)
{
  YAML::Node node = path_statistics::encode(rhs);
  if (rhs.id.has_value())
    node["id"] = rhs.id.value();
  else
    node["id"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::path_base::decode(
    const YAML::Node& node, ItineraryPgDao::path_base& rhs)
{
  if (node["id"] && !node["id"].IsNull())
    rhs.id = node["id"].as<long>();
  return path_statistics::decode(node, rhs);
}

YAML::Node
    ItineraryPgDao::path_summary::encode(const ItineraryPgDao::path_summary& rhs)
{
  YAML::Node node = ItineraryPgDao::path_base::encode(rhs);
  if (rhs.name.has_value())
    node["name"] = rhs.name.value();
  else
    node["name"] = YAML::Null;
  if (rhs.color_key.has_value())
    node["color"] = rhs.color_key.value();
  else
    node["color"] = YAML::Null;
  if (rhs.html_code.has_value())
    node["htmlcolor"] = rhs.html_code.value();
  else
    node["htmlcolor"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::path_summary::decode(const YAML::Node& node,
                                          ItineraryPgDao::path_summary& rhs)
{
  if (node["name"] && !node["name"].IsNull())
    rhs.name = node["name"].as<std::string>();
  if (node["color"] && !node["color"].IsNull())
    rhs.color_key = node["color"].as<std::string>();
  if (node["htmlcolor"] && !node["htmlcolor"].IsNull())
    rhs.html_code = node["htmlcolor"].as<std::string>();
  return fdsd::trip::ItineraryPgDao::path_base::decode(node, rhs);
}

YAML::Node ItineraryPgDao::route::encode(const ItineraryPgDao::route& rhs)
{
  auto node = fdsd::trip::ItineraryPgDao::path_summary::encode(rhs);
  node["points"] = rhs.points;
  return node;
}

bool ItineraryPgDao::route::decode(const YAML::Node& node,
                                   ItineraryPgDao::route& rhs)
{
  rhs.points = node["points"].as<std::vector<ItineraryPgDao::route_point>>();
  return fdsd::trip::ItineraryPgDao::path_summary::decode(node, rhs);
}

ItineraryPgDao::path_summary ItineraryPgDao::get_route_summary(
    std::string user_id,
    long itinerary_id,
    long route_id)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    auto rt = tx.exec_params1(
        "SELECT id, name, color, distance, ascent, descent, lowest, highest "
        "FROM itinerary_route WHERE itinerary_id=$1 AND id=$2",
        itinerary_id,
        route_id);
    route route;
    route.id = rt["id"].as<long>();
    if (!rt["name"].is_null())
      route.name = rt["name"].as<std::string>();
    if (!rt["color"].is_null())
      route.color_key = rt["color"].as<std::string>();
    if (!rt["distance"].is_null())
      route.distance = rt["distance"].as<double>();
    if (!rt["ascent"].is_null())
      route.ascent = rt["ascent"].as<double>();
    if (!rt["descent"].is_null())
      route.descent = rt["descent"].as<double>();
    if (!rt["lowest"].is_null())
      route.lowest = rt["lowest"].as<double>();
    if (!rt["highest"].is_null())
      route.highest = rt["highest"].as<double>();
    tx.commit();
    return route;
  } catch (const std::exception &e) {
    std::cerr << "Exception getting route summary: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::path_summary> ItineraryPgDao::get_route_summaries(
    std::string user_id,
    long itinerary_id,
    std::vector<long> route_ids)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    const std::string ids_sql = dao_helper::to_sql_array(route_ids);
    auto result = tx.exec_params(
        "SELECT id, name, color, html_code, c.value AS path_color_value, "
        "distance, ascent, descent, lowest, highest "
        "FROM itinerary_route r "
        "LEFT JOIN path_color c ON r.color=c.key "
        "WHERE itinerary_id=$1 AND id=ANY($2) "
        "ORDER BY name, id",
        itinerary_id,
        ids_sql);
    std::vector<path_summary> routes;
    for (const auto &rt : result) {
      path_summary route;
      route.id = rt["id"].as<long>();
      if (!rt["name"].is_null())
        route.name = rt["name"].as<std::string>();
      if (!rt["color"].is_null())
        route.color_key = rt["color"].as<std::string>();
      if (!rt["html_code"].is_null())
        route.html_code = rt["html_code"].as<std::string>();
      if (!rt["path_color_value"].is_null())
        route.color_description = rt["path_color_value"].as<std::string>();
      if (!rt["distance"].is_null())
        route.distance = rt["distance"].as<double>();
      if (!rt["ascent"].is_null())
        route.ascent = rt["ascent"].as<double>();
      if (!rt["descent"].is_null())
        route.descent = rt["descent"].as<double>();
      if (!rt["lowest"].is_null())
        route.lowest = rt["lowest"].as<double>();
      if (!rt["highest"].is_null())
        route.highest = rt["highest"].as<double>();
      routes.push_back(route);
    }
    tx.commit();
    return routes;
  } catch (const std::exception &e) {
    std::cerr << "Exception getting route summaries: "
              << e.what() << '\n';
    throw;
  }
}

ItineraryPgDao::path_summary ItineraryPgDao::get_track_summary(std::string user_id,
                                                        long itinerary_id,
                                                        long track_id)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    auto rt = tx.exec_params1(
        "SELECT id, name, color, html_code, distance, ascent, descent, "
        "lowest, highest "
        "FROM itinerary_track t LEFT JOIN path_color c ON t.color=c.key "
        "WHERE itinerary_id=$1 AND id=$2",
        itinerary_id,
        track_id);
    track track;
    track.id = rt["id"].as<long>();
    if (!rt["name"].is_null())
      track.name = rt["name"].as<std::string>();
    if (!rt["color"].is_null())
      track.color_key = rt["color"].as<std::string>();
    if (!rt["html_code"].is_null())
      track.html_code = rt["html_code"].as<std::string>();
    if (!rt["distance"].is_null())
      track.distance = rt["distance"].as<double>();
    if (!rt["ascent"].is_null())
      track.ascent = rt["ascent"].as<double>();
    if (!rt["descent"].is_null())
      track.descent = rt["descent"].as<double>();
    if (!rt["lowest"].is_null())
      track.lowest = rt["lowest"].as<double>();
    if (!rt["highest"].is_null())
      track.highest = rt["highest"].as<double>();
    tx.commit();
    return track;
  } catch (const std::exception &e) {
    std::cerr << "Exception getting track summary: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::path_summary> ItineraryPgDao::get_track_summaries(
    std::string user_id,
    long itinerary_id,
    std::vector<long> track_ids)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    const std::string ids_sql = dao_helper::to_sql_array(track_ids);
    auto result = tx.exec_params(
        "SELECT id, name, color, html_code, c.value AS path_color_value, "
        "distance, ascent, descent, "
        "lowest, highest "
        "FROM itinerary_track t LEFT JOIN path_color c ON t.color=c.key "
        "WHERE itinerary_id=$1 AND id=ANY($2) ORDER BY name, id",
        itinerary_id,
        ids_sql);
    std::vector<path_summary> tracks;
    for (const auto &trk : result) {
      path_summary track;
      track.id = trk["id"].as<long>();
      if (!trk["name"].is_null())
        track.name = trk["name"].as<std::string>();
      if (!trk["color"].is_null())
        track.color_key = trk["color"].as<std::string>();
      if (!trk["html_code"].is_null())
        track.html_code = trk["html_code"].as<std::string>();
      if (!trk["path_color_value"].is_null())
        track.color_description = trk["path_color_value"].as<std::string>();
      if (!trk["distance"].is_null())
        track.distance = trk["distance"].as<double>();
      if (!trk["ascent"].is_null())
        track.ascent = trk["ascent"].as<double>();
      if (!trk["descent"].is_null())
        track.descent = trk["descent"].as<double>();
      if (!trk["lowest"].is_null())
        track.lowest = trk["lowest"].as<double>();
      if (!trk["highest"].is_null())
        track.highest = trk["highest"].as<double>();
      tracks.push_back(track);
    }
    tx.commit();
    return tracks;
  } catch (const std::exception &e) {
    std::cerr << "Exception getting track summaries: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::path_color>
    ItineraryPgDao::get_path_colors()
{
  try {
    std::vector<std::pair<std::string, std::string>> colours;
    work tx(*connection);
    std::vector<path_color> colors;
    auto result = tx.exec_params(
        "SELECT key, value, html_code FROM path_color ORDER BY value");
    for (const auto &r : result) {
      path_color c;
      r["key"].to(c.key);
      r["value"].to(c.description);
      r["html_code"].to(c.html_code);
      colors.push_back(c);
    }
    tx.commit();
    return colors;
  } catch (const std::exception &e) {
    std::cerr << "Exception getting path color options: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<std::pair<std::string, std::string>>
    ItineraryPgDao::get_path_color_options()
{
  try {
    std::vector<std::pair<std::string, std::string>> colours;
    work tx(*connection);
    auto result = tx.exec_params(
        "SELECT key, value FROM path_color ORDER BY value");
    for (const auto &r : result) {
      const std::string key = r["key"].as<std::string>();
      const std::string value = r["value"].as<std::string>();
      colours.push_back(std::make_pair(key, value));
    }
    tx.commit();
    return colours;
  } catch (const std::exception &e) {
    std::cerr << "Exception getting path color options: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::update_route_summary(
    std::string user_id,
    long itinerary_id,
    const ItineraryPgDao::path_summary &route)
{
  if (!route.id.has_value())
    throw std::invalid_argument("Route ID not set");
  try {
    work tx(*connection);
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    tx.exec_params(
        "UPDATE itinerary_route "
        "SET name=$3, color=$4 "
        "WHERE id=$2 AND itinerary_id=$1",
        itinerary_id,
        route.id.value(),
        route.name,
        route.color_key
      );
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception updating route summary: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::update_track_summary(
    std::string user_id,
    long itinerary_id,
    const ItineraryPgDao::path_summary &track)
{
  if (!track.id.has_value())
    throw std::invalid_argument("Track ID not set");
  try {
    work tx(*connection);
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    tx.exec_params(
        "UPDATE itinerary_track "
        "SET name=$3, color=$4 "
        "WHERE id=$2 AND itinerary_id=$1",
        itinerary_id,
        track.id.value(),
        track.name,
        track.color_key
      );
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception updating track summary: "
              << e.what() << '\n';
    throw;
  }
}

long ItineraryPgDao::get_track_segment_count(std::string user_id,
                                             long itinerary_id,
                                             long track_id)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    auto r = tx.exec_params1(
        "SELECT COUNT(*) FROM itinerary_track_segment seg "
        "JOIN itinerary_track t ON t.id=seg.itinerary_track_id "
        "WHERE t.itinerary_id=$1 AND seg.itinerary_track_id=$2",
        itinerary_id,
        track_id);
    tx.commit();
    return r[0].as<long>();
  } catch (const std::logic_error& e) {
    std::cerr << "Error fetching track segment count: "
              << e.what() << '\n';
    throw;
  }
}

ItineraryPgDao::track
    ItineraryPgDao::get_track_segments(std::string user_id,
                                       long itinerary_id,
                                       long track_id,
                                       std::uint32_t offset,
                                       int limit)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    auto r = tx.exec_params1(
        "SELECT id, name, color AS color_key, c.value AS color_description, "
        "c.html_code, distance, ascent, "
        "descent, lowest, highest "
        "FROM itinerary_track t "
        "LEFT JOIN path_color c ON t.color=c.key "
        "WHERE t.itinerary_id=$1 AND t.id=$2",
        itinerary_id,
        track_id);
    track track;
    track.id = r["id"].as<long>();
    if (!r["name"].is_null())
      track.name = r["name"].as<std::string>();
    if (!r["color_key"].is_null())
      track.color_key = r["color_key"].as<std::string>();
    if (!r["color_description"].is_null())
      track.color_description = r["color_description"].as<std::string>();
    if (!r["html_code"].is_null())
      track.html_code = r["html_code"].as<std::string>();
    if (!r["distance"].is_null())
      track.distance = r["distance"].as<double>();
    if (!r["ascent"].is_null())
      track.ascent = r["ascent"].as<double>();
    if (!r["descent"].is_null())
      track.descent = r["descent"].as<double>();
    if (!r["lowest"].is_null())
      track.lowest = r["lowest"].as<double>();
    if (!r["highest"].is_null())
      track.highest = r["highest"].as<double>();
    auto result = tx.exec_params(
        "SELECT id, distance, ascent, descent, lowest, highest "
        "FROM itinerary_track_segment WHERE itinerary_track_id=$1 "
        "ORDER BY id OFFSET $2 LIMIT $3",
        track_id,
        offset,
        limit
      );
    for (const auto &r : result) {
      track_segment trkseg;
      trkseg.id = r["id"].as<long>();
      if (!r["distance"].is_null())
        trkseg.distance = r["distance"].as<double>();
      if (!r["ascent"].is_null())
        trkseg.ascent = r["ascent"].as<double>();
      if (!r["descent"].is_null())
        trkseg.descent = r["descent"].as<double>();
      if (!r["lowest"].is_null())
        trkseg.lowest = r["lowest"].as<double>();
      if (!r["highest"].is_null())
        trkseg.highest = r["highest"].as<double>();
      if (trkseg.id.has_value())
        track.segments.push_back(trkseg);
    }
    tx.commit();
    return track;
  } catch (const std::logic_error& e) {
    std::cerr << "Error fetching track segments: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::track_point>
    ItineraryPgDao::get_track_points(
        std::string user_id,
        long itinerary_id,
        const std::vector<long> &point_ids)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    const std::string ids_sql = dao_helper::to_sql_array(point_ids);
    auto result = tx.exec_params(
        "SELECT tp.id, tp.time, ST_X(tp.geog::geometry) as lng, "
        "ST_Y(tp.geog::geometry) as lat, tp.altitude, tp.hdop "
        "FROM itinerary_track_point tp "
        "JOIN itinerary_track_segment ts ON tp.itinerary_track_segment_id=ts.id "
        "JOIN itinerary_track t ON ts.itinerary_track_id=t.id "
        "WHERE t.itinerary_id=$1 AND tp.id=ANY($2) "
        "ORDER BY tp.id",
        itinerary_id,
        ids_sql);
    std::vector<track_point> retval;
    for (const auto &r : result) {
      track_point p;
      p.id = r["id"].as<long>();
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      std::string s;
      if (r["time"].to(s)) {
        DateTime time(s);
        p.time = time.time_tp();
      }
      if (!r["hdop"].is_null())
        p.hdop = r["hdop"].as<double>();
      if (!r["altitude"].is_null())
        p.altitude = r["altitude"].as<double>();
      retval.push_back(p);
    }
    tx.commit();
    return retval;
  } catch (const std::logic_error& e) {
    std::cerr << "Error fetching track points: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::route_point>
    ItineraryPgDao::get_route_points(
        std::string user_id,
        long itinerary_id,
        const std::vector<long> &point_ids)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    const std::string ids_sql = dao_helper::to_sql_array(point_ids);
    auto result = tx.exec_params(
        "SELECT p.id, ST_X(p.geog::geometry) as lng, "
        "ST_Y(p.geog::geometry) as lat, p.altitude "
        "FROM itinerary_route_point p "
        "JOIN itinerary_route rt ON p.itinerary_route_id=rt.id "
        "WHERE rt.itinerary_id=$1 AND p.id=ANY($2) "
        "ORDER BY p.id",
        itinerary_id,
        ids_sql);
    std::vector<route_point> retval;
    for (const auto &r : result) {
      route_point p;
      p.id = r["id"].as<long>();
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      if (!r["altitude"].is_null())
        p.altitude = r["altitude"].as<double>();
      retval.push_back(p);
    }
    tx.commit();
    return retval;
  } catch (const std::logic_error& e) {
    std::cerr << "Error fetching route points: "
              << e.what() << '\n';
    throw;
  }
}

long ItineraryPgDao::get_track_segment_point_count(
    std::string user_id,
    long itinerary_id,
    long track_segment_id)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    auto r = tx.exec_params1(
        "SELECT COUNT(*) FROM itinerary_track_point tp "
        "JOIN itinerary_track_segment ts ON tp.itinerary_track_segment_id=ts.id "
        "JOIN itinerary_track t ON ts.itinerary_track_id=t.id "
        "WHERE t.itinerary_id=$1 AND tp.itinerary_track_segment_id=$2",
        itinerary_id,
        track_segment_id);
    tx.commit();
    return r[0].as<long>();
  } catch (const std::logic_error& e) {
    std::cerr << "Error fetching track segment point count: "
              << e.what() << '\n';
    throw;
  }
}

long ItineraryPgDao::get_route_point_count(
    std::string user_id,
    long itinerary_id,
    long route_id)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    auto r = tx.exec_params1(
        "SELECT COUNT(*) "
        "FROM itinerary_route_point p "
        "JOIN itinerary_route r ON r.id=p.itinerary_route_id "
        "WHERE r.itinerary_id=$1 AND p.itinerary_route_id=$2 ",
        itinerary_id,
        route_id);
    tx.commit();
    return r[0].as<long>();
  } catch (const std::exception& e) {
    std::cerr << "Error fetching route point count: "
              << e.what() << '\n';
    throw;
  }
}

ItineraryPgDao::route ItineraryPgDao::get_route_points(
    std::string user_id,
    long itinerary_id,
    long route_id,
    std::uint32_t offset,
    int limit)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    auto r = tx.exec_params1(
        "SELECT r.id AS route_id, r.name AS route_name, r.color AS path_color, "
        "rc.value AS path_color_value, "
        "rc.html_code, r.distance, r.ascent, r.descent, r.lowest, r.highest "
        "FROM itinerary_route r "
        "JOIN itinerary i ON i.id=r.itinerary_id "
        "LEFT JOIN path_color rc ON r.color=rc.key "
        "WHERE r.itinerary_id=$1 AND r.id=$2 ",
        itinerary_id,
        route_id);

    ItineraryPgDao::route route;
    route.id = route_id;
    if (!r["route_name"].is_null())
      route.name = r["route_name"].as<std::string>();
    if (!r["path_color"].is_null())
      route.color_key = r["path_color"].as<std::string>();
    if (!r["path_color_value"].is_null())
      route.color_description = r["path_color_value"].as<std::string>();
    if (!r["html_code"].is_null())
      route.html_code = r["html_code"].as<std::string>();
    if (!r["distance"].is_null())
      route.distance = r["distance"].as<double>();
    if (!r["ascent"].is_null())
      route.ascent = r["ascent"].as<double>();
    if (!r["descent"].is_null())
      route.descent = r["descent"].as<double>();
    if (!r["lowest"].is_null())
      route.lowest = r["lowest"].as<double>();
    if (!r["highest"].is_null())
      route.highest = r["highest"].as<double>();
    
    std::optional<int> fetch_limit;
    if (limit > 0)
      fetch_limit = limit;
    auto result = tx.exec_params(
        "SELECT p.id AS point_id, "
        "ST_X(p.geog::geometry) as lng, ST_Y(p.geog::geometry) as lat, "
        "p.name AS point_name, p.comment, p.description, p.symbol, p.altitude "
        "FROM itinerary_route_point p "
        "WHERE p.itinerary_route_id=$1 "
        "ORDER BY p.id OFFSET $2 LIMIT $3",
        route_id,
        offset,
        fetch_limit);

    for (const auto &r : result) {
      route_point p;
      p.id = r["point_id"].as<long>();
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      if (!r["point_name"].is_null())
        p.name = r["point_name"].as<std::string>();
      if (!r["comment"].is_null())
        p.comment = r["comment"].as<std::string>();
      if (!r["description"].is_null())
        p.description = r["description"].as<std::string>();
      if (!r["symbol"].is_null())
        p.symbol = r["symbol"].as<std::string>();
      if (!r["altitude"].is_null())
        p.altitude = r["altitude"].as<double>();
      route.points.push_back(p);
    }
    tx.commit();
    return route;
  } catch (const std::exception& e) {
    std::cerr << "Error fetching route points: "
              << e.what() << '\n';
    throw;
  }
}

ItineraryPgDao::track_segment ItineraryPgDao::get_track_segment(
    std::string user_id,
    long itinerary_id,
    long track_segment_id,
    std::uint32_t offset,
    int limit)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    // Get the track segment validating that it belongs to both the track_id and
    // itinerary_id
    auto r = tx.exec_params1(
        "SELECT ts.id, ts.distance, ts.ascent, ts.descent, ts.lowest, ts.highest "
        "FROM itinerary_track_segment ts "
        "JOIN itinerary_track t ON t.id=ts.itinerary_track_id "
        "WHERE t.itinerary_id=$1 AND ts.id=$2",
        itinerary_id,
        track_segment_id
      );
    track_segment trkseg;
    trkseg.id = r["id"].as<long>();
    if (!r["distance"].is_null())
      trkseg.distance = r["distance"].as<double>();
    if (!r["ascent"].is_null())
      trkseg.ascent = r["ascent"].as<double>();
    if (!r["descent"].is_null())
      trkseg.descent = r["descent"].as<double>();
    if (!r["lowest"].is_null())
      trkseg.lowest = r["lowest"].as<double>();
    if (!r["highest"].is_null())
      trkseg.highest = r["highest"].as<double>();
    std::optional<int> fetch_limit;
    if (limit > 0)
      fetch_limit = limit;
    auto result = tx.exec_params(
        "SELECT tp.id, tp.time, ST_X(tp.geog::geometry) as lng, "
        "ST_Y(tp.geog::geometry) as lat, tp.altitude, tp.hdop "
        "FROM itinerary_track_point tp "
        "JOIN itinerary_track_segment ts ON tp.itinerary_track_segment_id=ts.id "
        "JOIN itinerary_track t ON ts.itinerary_track_id=t.id "
        "WHERE t.itinerary_id=$1 AND tp.itinerary_track_segment_id=$2 "
        "ORDER BY tp.id OFFSET $3 LIMIT $4",
        itinerary_id,
        track_segment_id,
        offset,
        fetch_limit
      );
    for (const auto &r : result) {
      track_point p;
      p.id = r["id"].as<long>();
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      std::string s;
      if (r["time"].to(s)) {
        DateTime time(s);
        p.time = time.time_tp();
      }
      if (!r["hdop"].is_null())
        p.hdop = r["hdop"].as<double>();
      if (!r["altitude"].is_null())
        p.altitude = r["altitude"].as<double>();
      trkseg.points.push_back(p);
    }
    tx.commit();
    return trkseg;
  } catch (const std::logic_error& e) {
    std::cerr << "Error fetching track segment: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::track_segment> ItineraryPgDao::get_track_segments(
    std::string user_id,
    long itinerary_id,
    long track_id,
    std::vector<long> track_segment_ids)
{
  try {
    work tx(*connection);
    validate_user_itinerary_read_access(tx, user_id, itinerary_id);
    const std::string ids_sql = dao_helper::to_sql_array(track_segment_ids);
    // Get the track segment validating that it belongs to both the track_id and
    // itinerary_id
    auto segments_result = tx.exec_params(
        "SELECT ts.id, ts.distance, ts.ascent, ts.descent, ts.lowest, ts.highest "
        "FROM itinerary_track_segment ts "
        "JOIN itinerary_track t ON t.id=ts.itinerary_track_id "
        "WHERE t.itinerary_id=$1 AND t.id=$2 AND ts.id=ANY($3) "
        "ORDER BY ts.id",
        itinerary_id,
        track_id,
        ids_sql
      );
    // use prepared statement to fetch each of the sets of points
    connection->prepare(
        "fetch-segment-points",
        "SELECT tp.id, tp.time, ST_X(tp.geog::geometry) as lng, "
        "ST_Y(tp.geog::geometry) as lat, tp.altitude, tp.hdop "
        "FROM itinerary_track_point tp "
        "JOIN itinerary_track_segment ts ON tp.itinerary_track_segment_id=ts.id "
        "JOIN itinerary_track t ON ts.itinerary_track_id=t.id "
        "WHERE t.itinerary_id=$1 AND tp.itinerary_track_segment_id=$2 "
        "ORDER BY tp.id");
    std::vector<track_segment> retval;
    for (const auto &seg : segments_result) {
      track_segment trkseg;
      trkseg.id = seg["id"].as<long>();
      if (trkseg.id.has_value()) {
        if (!seg["distance"].is_null())
          trkseg.distance = seg["distance"].as<double>();
        if (!seg["ascent"].is_null())
          trkseg.ascent = seg["ascent"].as<double>();
        if (!seg["descent"].is_null())
          trkseg.descent = seg["descent"].as<double>();
        if (!seg["lowest"].is_null())
          trkseg.lowest = seg["lowest"].as<double>();
        if (!seg["highest"].is_null())
          trkseg.highest = seg["highest"].as<double>();
        auto result = tx.exec_prepared(
            "fetch-segment-points",
            itinerary_id,
            trkseg.id.value()
          );
        for (const auto &r : result) {
          track_point p;
          p.id = r["id"].as<long>();
          r["lng"].to(p.longitude);
          r["lat"].to(p.latitude);
          std::string s;
          if (r["time"].to(s)) {
            DateTime time(s);
            p.time = time.time_tp();
          }
          if (!r["hdop"].is_null())
            p.hdop = r["hdop"].as<double>();
          if (!r["altitude"].is_null())
            p.altitude = r["altitude"].as<double>();
          trkseg.points.push_back(p);
        }
        retval.push_back(trkseg);
      }
    }
    tx.commit();
    return retval;
  } catch (const std::exception &e) {
    std::cerr << "Exception getting track segments: "
              << e.what() << '\n';
    throw;
  }
}

void ItineraryPgDao::save_updated_statistics(
    std::string user_id,
    long itinerary_id,
    const ItineraryPgDao::track &track)
{
  if (!track.id.has_value())
    throw std::invalid_argument("Track ID not set");
  try {
    work tx(*connection);
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    tx.exec_params(
        "UPDATE itinerary_track "
        "SET distance=$3, ascent=$4, descent=$5, lowest=$6, highest=$7 "
        "WHERE id=$2 AND itinerary_id=$1",
        itinerary_id,
        track.id.value(),
        track.distance,
        track.ascent,
        track.descent,
        track.lowest,
        track.highest
      );
    const std::string ps_name = "update_track_segment_stats";
    connection->prepare(
        ps_name,
        "UPDATE itinerary_track_segment "
        "SET distance=$3, ascent=$4, descent=$5, lowest=$6, highest=$7 "
        "WHERE id=$2 AND itinerary_track_id=$1"
      );
    for (const auto &ts : track.segments) {
      if (!ts.id.has_value())
        throw std::invalid_argument("Track segment ID not set");
      tx.exec_prepared(
          ps_name,
          track.id,
          ts.id,
          ts.distance,
          ts.ascent,
          ts.descent,
          ts.lowest,
          ts.highest
        );
    }
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception saving updated track statistics: "
              << e.what() << '\n';
    throw;
  }
}

const std::string ItineraryPgDao::itinerary_waypoint_radius_clause =
  "SELECT DISTINCT(i.id), i.start, i.finish, i.title, null AS nickname FROM itinerary i "
  "JOIN itinerary_waypoint iw ON iw.itinerary_id=i.id "
  "WHERE i.archived != true AND i.user_id=$1 "
  "AND ST_DWithin(iw.geog, ST_MakePoint($2,$3), $4) ";

const std::string ItineraryPgDao::itinerary_route_radius_clause =
  "SELECT DISTINCT(i.id), i.start, i.finish, i.title, null AS nickname FROM itinerary i "
  "JOIN itinerary_route ir ON ir.itinerary_id=i.id "
  "JOIN itinerary_route_point irp ON irp.itinerary_route_id=ir.id "
  "WHERE i.archived != true AND i.user_id=$1 "
  "AND ST_DWithin(irp.geog, ST_MakePoint($2,$3), $4) ";

const std::string ItineraryPgDao::itinerary_track_radius_clause =
  "SELECT DISTINCT(i.id), i.start, i.finish, i.title, null AS nickname FROM itinerary i "
  "JOIN itinerary_track it ON it.itinerary_id=i.id "
  "JOIN itinerary_track_segment its ON its.itinerary_track_id=it.id "
  "JOIN itinerary_track_point itp ON itp.itinerary_track_segment_id=its.id "
  "WHERE i.archived != true AND i.user_id=$1 "
  "AND ST_DWithin(itp.geog, ST_MakePoint($2,$3), $4) ";

const std::string ItineraryPgDao::shared_itinerary_waypoint_radius_clause =
  "SELECT DISTINCT(i2.id), i2.start, i2.finish, i2.title, u3.nickname FROM itinerary i2 "
  "JOIN itinerary_sharing s ON i2.id=s.itinerary_id "
  "JOIN usertable u2 ON s.shared_to_id=u2.id "
  "JOIN itinerary_waypoint iw ON i2.id=iw.itinerary_id "
  "JOIN usertable u3 ON u3.id=i2.user_id "
  "WHERE i2.archived != true AND s.active=true AND u2.id=$1 "
  "AND ST_DWithin(iw.geog, ST_MakePoint($2,$3), $4) ";

const std::string ItineraryPgDao::shared_itinerary_route_radius_clause =
  "SELECT DISTINCT(i2.id), i2.start, i2.finish, i2.title, u3.nickname FROM itinerary i2 "
  "JOIN itinerary_sharing s ON i2.id=s.itinerary_id "
  "JOIN usertable u2 ON s.shared_to_id=u2.id "
  "JOIN itinerary_route ir ON i2.id=ir.itinerary_id "
  "JOIN itinerary_route_point irp ON ir.id=irp.itinerary_route_id "
  "JOIN usertable u3 ON u3.id=i2.user_id "
  "WHERE i2.archived != true AND s.active=true AND u2.id=$1 "
  "AND ST_DWithin(irp.geog, ST_MakePoint($2,$3), $4) ";

const std::string ItineraryPgDao::shared_itinerary_track_radius_clause =
  "SELECT DISTINCT(i2.id), i2.start, i2.finish, i2.title, u3.nickname FROM itinerary i2 "
  "JOIN itinerary_sharing s ON i2.id=s.itinerary_id "
  "JOIN usertable u2 ON s.shared_to_id=u2.id "
  "JOIN itinerary_track it ON i2.id=it.itinerary_id "
  "JOIN itinerary_track_segment its ON it.id=its.itinerary_track_id "
  "JOIN itinerary_track_point itp ON its.id=itp.itinerary_track_segment_id "
  "JOIN usertable u3 ON u3.id=i2.user_id "
  "WHERE i2.archived != true AND s.active=true AND u2.id=$1 "
  "AND ST_DWithin(itp.geog, ST_MakePoint($2,$3), $4) ";

const std::string ItineraryPgDao::itinerary_search_query_body =
  itinerary_waypoint_radius_clause + " UNION " +
  itinerary_route_radius_clause + " UNION " +
  itinerary_track_radius_clause + " UNION " +
  shared_itinerary_waypoint_radius_clause + " UNION " +
  shared_itinerary_route_radius_clause + " UNION " +
  shared_itinerary_track_radius_clause;

long ItineraryPgDao::itinerary_radius_search_count(
    std::string user_id,
    double longitude,
    double latitude,
    double radius)
{
  try {
    work tx(*connection);
    const std::string sql =
      "SELECT COUNT(*) FROM (" +
      itinerary_search_query_body +
      ") AS q";
    auto r = tx.exec_params1(
        sql,
        user_id,
        longitude,
        latitude,
        radius
      );
    tx.commit();
    return r[0].as<long>();
  } catch (const std::exception &e) {
    std::cerr << "Error executing itinerary radius search count: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::itinerary_summary>
    ItineraryPgDao::itinerary_radius_search(
        std::string user_id,
        double longitude,
        double latitude,
        double radius,
        std::uint32_t offset,
        int limit)
{
  try {
    work tx(*connection);
    const std::string sql =
      itinerary_search_query_body +
      "ORDER BY start DESC, finish DESC, title, id DESC "
      "OFFSET $5 LIMIT $6";
    auto result = tx.exec_params(
        sql,
        user_id,
        longitude,
        latitude,
        radius,
        offset,
        limit
      );
    std::vector<ItineraryPgDao::itinerary_summary> itineraries;
    for (auto r : result) {
      itinerary_summary it;
      it.id = r["id"].as<long>();
      std::string s;
      if (r["start"].to(s)) {
        DateTime start(s);
        it.start = start.time_tp();
      }
      if (r["finish"].to(s)) {
        DateTime finish(s);
        it.finish = finish.time_tp();
      }
      r["title"].to(it.title);
      if (!r["nickname"].is_null())
        it.owner_nickname = r["nickname"].as<std::string>();
      it.shared = it.owner_nickname.has_value() && !it.owner_nickname.value().empty();
      itineraries.push_back(it);
    }
    tx.commit();
    return itineraries;
  } catch (const std::exception &e) {
    std::cerr << "Error executing itinerary radius search: "
              << e.what() << '\n';
    throw;
  }
}

long ItineraryPgDao::get_shared_itinerary_report_count(
        std::string user_id)
{
  try {
    const std::string sql =
      "SELECT COUNT(*) FROM ("
      "SELECT DISTINCT(it.id) FROM itinerary it "
      "JOIN itinerary_sharing sh ON it.id=sh.itinerary_id "
      "WHERE it.archived != true AND sh.active AND it.user_id=$1) AS q";
    work tx(*connection);
    auto r = tx.exec_params1(
        sql,
        user_id);
    return r[0].as<long>();
  } catch (const std::exception &e) {
    std::cerr << "Error fetching itinerary share report count: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<ItineraryPgDao::itinerary_share_report>
    ItineraryPgDao::get_shared_itinerary_report(
        std::string user_id,
        std::uint32_t offset,
        int limit)
{
  try {
    connection->prepare(
        "itinerary-share-nicknames",
        "SELECT nickname FROM itinerary_sharing sh "
        "JOIN usertable u ON u.id=shared_to_id WHERE sh.active AND itinerary_id=$1");
    const std::string sql =
      "SELECT DISTINCT(it.id), title, start FROM itinerary it "
      "JOIN itinerary_sharing sh ON it.id=sh.itinerary_id "
      "WHERE it.archived != true AND sh.active AND it.user_id=$1 "
      "ORDER BY start DESC, it.id OFFSET $2 LIMIT $3";
    // std::cout << "SQL: " << sql << '\n';
    work tx(*connection);
    auto result = tx.exec_params(
        sql,
        user_id,
        offset,
        limit);
    std::vector<ItineraryPgDao::itinerary_share_report> itineraries;
    for (const auto &r : result) {
      itinerary_share_report i;
      i.id =  r["id"].as<long>();
      r["title"].to(i.title);
      std::string s;
      if (r["start"].to(s)) {
        DateTime start(s);
        i.start = start.time_tp();
      }
      auto nicknames = tx.exec_prepared(
          "itinerary-share-nicknames",
          i.id.value());
      for (const auto &n : nicknames) {
        i.nicknames.push_back(n["nickname"].as<std::string>());
      }
      itineraries.push_back(i);
    }
    return itineraries;
  } catch (const std::exception &e) {
    std::cerr << "Error fetching itinerary share report: "
              << e.what() << '\n';
    throw;
  }
}

std::vector<std::string>
    ItineraryPgDao::get_nicknames_sharing_location_with_user(
        std::string user_id)
{
  try {
    work tx(*connection);
    auto result = tx.exec_params(
        "SELECT shared_by_id AS id, u.nickname "
        "FROM location_sharing loc "
        "JOIN usertable u ON u.id=shared_by_id "
        "WHERE active=true AND shared_to_id=$1 UNION ALL "
        "SELECT id, nickname FROM usertable u2 "
        "WHERE u2.id=$1 ORDER BY nickname",
        user_id);
    std::vector<std::string> retval;
    for (const auto &r : result)
      retval.push_back(r["nickname"].as<std::string>());
    tx.commit();
    return retval;
  } catch (const std::exception &e) {
    std::cerr << "Error fetching nicknames sharing location with user: "
              << e.what() << '\n';
    throw;
  }
}

void time_span_type::update_start(
    const std::chrono::system_clock::time_point other_start)
{
  if (!is_valid) {
    start = other_start;
    finish = start;
  } else {
    start = std::min(start, other_start);
  }
  is_valid = true;
}

void time_span_type::update_finish(
    const std::chrono::system_clock::time_point other_finish)
{
  if (!is_valid) {
    finish = other_finish;
    start = finish;
  } else {
    finish = std::max(finish, other_finish);
  }
  is_valid = true;
}

/// Updates this time span to encompass the range of the passed time span
void time_span_type::update_time_span(const time_span_type &other) {
  if (other.is_valid) {
    update_start(other.start);
    update_finish(other.finish);
  }
}

void ItineraryPgDao::update_time_span(
    time_span_type &time_span,
    const std::vector<ItineraryPgDao::track> &tracks)
{
  for (const auto &t : tracks)
    for (const auto &ts : t.segments)
      for (const auto &p : ts.points)
        if (p.time.has_value())
          time_span.update(p.time.value());
}

void ItineraryPgDao::update_time_span(
    time_span_type &time_span,
    const std::vector<ItineraryPgDao::waypoint> &waypoints)
{
  for (const auto &w : waypoints)
    if (w.time.has_value())
      time_span.update(w.time.value());
}

time_span_type ItineraryPgDao::get_time_span(
    const std::vector<ItineraryPgDao::track> &tracks,
    const std::vector<ItineraryPgDao::waypoint> &waypoints)
{
  time_span_type time_span;
  update_time_span(time_span, tracks);
  update_time_span(time_span, waypoints);
  return time_span;
}

std::optional<bounding_box> ItineraryPgDao::get_bounding_box(
    const std::vector<ItineraryPgDao::track> &tracks,
    const std::vector<ItineraryPgDao::route> &routes,
    const std::vector<ItineraryPgDao::waypoint> &waypoints)
{
  std::unique_ptr<bounding_box> box;
  for (const auto &t : tracks)
    for (const auto &ts : t.segments)
      for (const auto &p : ts.points)
        if (box)
          box->extend(p);
        else
          box = std::unique_ptr<bounding_box>(new bounding_box(p));
  for (const auto &r : routes)
    for (const auto &p : r.points)
      if (box)
        box->extend(p);
      else
        box = std::unique_ptr<bounding_box>(new bounding_box(p));
  for (const auto &p : waypoints)
    if (box)
      box->extend(p);
    else
      box = std::unique_ptr<bounding_box>(new bounding_box(p));
  if (!box)
    return bounding_box();

  return bounding_box(*box);
}
