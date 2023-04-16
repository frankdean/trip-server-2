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
#include "itinerary_pg_dao.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include "../trip-server-common/src/dao_helper.hpp"
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

void ItineraryPgDao::path_summary::from_geojson_properties(
    const nlohmann::json& properties,
    ItineraryPgDao::path_summary &path)
{
  const auto name_iter = properties.find("name");
  if ((path.name.first = name_iter != properties.cend()))
    name_iter->get_to(path.name.second);
  const auto html_color_iter = properties.find("html_color_code");
  if ((path.html_code.first = html_color_iter != properties.cend()))
    html_color_iter->get_to(path.html_code.second);
  const auto color_key_iter = properties.find("color_code");
  if ((path.color_key.first = color_key_iter != properties.cend()))
    color_key_iter->get_to(path.color_key.second);
}

ItineraryPgDao::route::route(const track &t) : path_summary(t), points()
{
  id.first = false;
  // We lose separate segments from the track
  for (const auto &ts : t.segments) {
    for (const auto &tp : ts.points) {
      // We lose time and hdop from the track point
      route_point rp(tp);
      rp.id.first = false;
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
  id.first = false;
  // Single segment for the track
  track_segment ts;
  for (const auto &rp : r.points) {
    // We lose name, comment, description and symbol from the route point
    track_point tp(rp);
    tp.id.first = false;
    tp.time = std::make_pair(true, time);
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
      it.id.first = r["id"].to(it.id.second);
      std::string s;
      if ((it.start.first = r["start"].to(s))) {
        DateTime start(s);
        it.start.second = start.time_tp();
      }
      if ((it.finish.first = r["finish"].to(s))) {
        DateTime finish(s);
        it.finish.second = finish.time_tp();
      }
      r["title"].to(it.title);
      it.owner_nickname.first = r["nickname"].to(it.owner_nickname.second);
      it.shared.first = r["shared"].to(it.shared.second);
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

std::pair<bool, ItineraryPgDao::itinerary> ItineraryPgDao::get_itinerary_summary(
    std::string user_id, long itinerary_id)
{
  try {
    work tx(*connection);
    auto details = get_itinerary_details(tx, user_id, itinerary_id);
    if (details.first) {
      ItineraryPgDao::itinerary it(details.second);
      // Query routes, tracks and waypoints
      auto route_result = tx.exec_params(
          "SELECT r.id, r.name, r.color AS color_key, "
          "rc.value AS color_description, "
          "distance, ascent, descent, lowest, highest "
          "FROM itinerary_route r "
          "JOIN path_color rc ON r.color=rc.key "
          "WHERE itinerary_id=$1 ORDER BY name, id",
          itinerary_id);
      for (const auto &rt : route_result) {
        path_summary route;
        route.id.first = rt["id"].to(route.id.second);
        route.name.first = rt["name"].to(route.name.second);
        route.color_key.first = rt["color_key"].to(route.color_key.second);
        route.color_description.first =
          rt["color_description"].to(route.color_description.second);
        route.distance.first = rt["distance"].to(route.distance.second);
        route.ascent.first = rt["ascent"].to(route.ascent.second);
        route.descent.first = rt["descent"].to(route.descent.second);
        route.lowest.first = rt["lowest"].to(route.lowest.second);
        route.highest.first = rt["highest"].to(route.highest.second);
        it.routes.push_back(route);
      }
      auto track_result = tx.exec_params(
          "SELECT t.id, t.name, t.color AS color_key, "
          "rc.value AS color_description, "
          "distance, ascent, descent, lowest, highest "
          "FROM itinerary_track t "
          "JOIN path_color rc ON t.color=rc.key "
          "WHERE itinerary_id=$1 ORDER BY name, id",
          itinerary_id);
      for (const auto &tk : track_result) {
        path_summary track;
        track.id.first = tk["id"].to(track.id.second);
        track.name.first = tk["name"].to(track.name.second);
        track.color_key.first = tk["color_key"].to(track.color_key.second);
        track.color_description.first = tk["color_description"].to(track.color_description.second);
        track.distance.first = tk["distance"].to(track.distance.second);
        track.ascent.first = tk["ascent"].to(track.ascent.second);
        track.descent.first = tk["descent"].to(track.descent.second);
        track.lowest.first = tk["lowest"].to(track.lowest.second);
        track.highest.first = tk["highest"].to(track.highest.second);
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
        waypoint.name.first = wpt["name"].to(waypoint.name.second);
        waypoint.comment.first = wpt["comment"].to(waypoint.comment.second);
        waypoint.type.first = wpt["type"].to(waypoint.type.second);
        waypoint.symbol.first = wpt["symbol_text"].to(waypoint.symbol.second);
        if (!waypoint.symbol.first)
          waypoint.symbol.first = wpt["symbol"].to(waypoint.symbol.second);
        it.waypoints.push_back(waypoint);
      }

      tx.commit();
      return std::make_pair(true, it);
    } else {
      return std::make_pair(false, ItineraryPgDao::itinerary());
    }
  } catch (const std::exception &e) {
    std::cerr << "Exception executing query to fetch itinerary details: "
              << e.what() << '\n';
    throw;
  }
}

std::pair<bool, ItineraryPgDao::itinerary_complete>
    ItineraryPgDao::get_itinerary_complete(
        std::string user_id, long itinerary_id)
{
  try {
    work tx(*connection);
    auto itinerary_details =
      get_itinerary_details(tx, user_id, itinerary_id);
    ItineraryPgDao::itinerary_complete itinerary(itinerary_details.second);

    if (itinerary_details.first) {

      // Don't leak details of whom another user may also be sharing an
      // itinerary with.
      if (has_user_itinerary_modification_access(tx, user_id, itinerary_id))
          itinerary.shares = get_itinerary_shares(tx, user_id, itinerary_id);

      itinerary.routes = get_routes(tx, user_id, itinerary_id);
      itinerary.waypoints = get_waypoints(tx, user_id, itinerary_id);
      itinerary.tracks = get_tracks(tx, user_id, itinerary_id);
    }

    return std::make_pair(itinerary_details.first, itinerary);

  } catch (const std::exception &e) {
    std::cerr
      << "Exception executing query to fetch complete itinerary details: "
      << e.what() << '\n';
    throw;
  }
}

std::pair<bool, ItineraryPgDao::itinerary_description>
    ItineraryPgDao::get_itinerary_description(
        std::string user_id, long itinerary_id)
{
  ItineraryPgDao::itinerary_description it;
  try {
    work tx(*connection);
    auto r =tx.exec_params1(
        "SELECT id, start, finish, title, description "
        "FROM itinerary "
        "WHERE archived != true AND user_id=$1 AND id=$2"
        ,
        user_id,
        itinerary_id
      );
    it.id.first = r["id"].to(it.id.second);
    std::string s;
    if ((it.start.first = r["start"].to(s))) {
      DateTime start(s);
      it.start.second = start.time_tp();
    }
    if ((it.finish.first = r["finish"].to(s))) {
      DateTime finish(s);
      it.finish.second = finish.time_tp();
    }
    r["title"].to(it.title);
    it.description.first = r["description"].to(it.description.second);
    tx.commit();
    return std::make_pair(true, it);
  } catch (const pqxx::unexpected_rows) {
    return std::make_pair(false, it);
  } catch (const std::exception &e) {
    std::cerr << "Exception executing query to fetch itinerary description: "
              << e.what() << '\n';
    throw;
  }
}

std::pair<bool, ItineraryPgDao::itinerary_detail>
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
    "WHERE i2.id=$2 AND s.active=true AND u2.id=$1 "
    "ORDER BY shared_to_nickname DESC";

  // std::cout << "SQL:: " << sql << '\n';
  auto result = tx.exec_params(
      sql,
      user_id,
      itinerary_id
    );
  ItineraryPgDao::itinerary_detail it;
  if (result.empty()) {
    return std::make_pair(false, it);
  }
  auto r = result[0];
  it.id.first = r["id"].to(it.id.second);
  std::string s;
  if ((it.start.first = r["start"].to(s))) {
    DateTime start(s);
    it.start.second = start.time_tp();
  }
  if ((it.finish.first = r["finish"].to(s))) {
    DateTime finish(s);
    it.finish.second = finish.time_tp();
  }
  r["title"].to(it.title);
  it.owner_nickname.first = r["owned_by_nickname"].to(it.owner_nickname.second);
  it.shared_to_nickname.first =
    r["shared_to_nickname"].to(it.shared_to_nickname.second);
  it.shared.first = (it.shared.second = it.shared_to_nickname.first);
  it.description.first = r["description"].to(it.description.second);
  return std::make_pair(true, it);
}

long ItineraryPgDao::save(
    std::string user_id,
    ItineraryPgDao::itinerary_description itinerary)
{
  try {
    work tx(*connection);
    std::string start;
    std::string finish;
    if (itinerary.start.first)
      start = DateTime(itinerary.start.second).get_time_as_iso8601_gmt();
    if (itinerary.finish.first)
      finish = DateTime(itinerary.finish.second).get_time_as_iso8601_gmt();
    std::string sql;
    long id;
    if (itinerary.id.first) {
      id = itinerary.id.second;
      tx.exec_params(
          "UPDATE itinerary "
          "SET title=$3, start=$4, finish=$5, description=$6 "
          "WHERE user_id=$1 AND id=$2",
          user_id,
          itinerary.id.first ? &itinerary.id.second : nullptr,
          itinerary.title,
          itinerary.start.first ? &start : nullptr,
          itinerary.finish.first ? &finish : nullptr,
          itinerary.description.first ? &itinerary.description.second : nullptr
        );
    } else {
      auto r = tx.exec_params1(
          "INSERT INTO itinerary "
          "(user_id, title, start, finish, description) "
          "VALUES ($1, $2, $3, $4, $5) RETURNING id",
          user_id,
          itinerary.title,
          itinerary.start.first ? &start : nullptr,
          itinerary.finish.first ? &finish : nullptr,
          itinerary.description.first ? &itinerary.description.second : nullptr
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
      "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1"
      "WHERE (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
      "AND r.itinerary_id = $2 AND r.id=ANY($3) "
      "ORDER BY r.id, p.id",
      user_id,
      itinerary_id,
      route_ids_sql
    );
  route rt;
  for (const auto &r : result) {
    long route_id = r["route_id"].as<long>();
    if (!rt.id.first || rt.id.second != route_id) {
      if (rt.id.first) {
        routes.push_back(rt);
      }
      rt = route();
      rt.id.first = true;
      rt.id.second = route_id;
      rt.name.first = r["route_name"].to(rt.name.second);
      rt.color_key.first = r["path_color_key"].to(rt.color_key.second);
      rt.color_description.first = r["path_color_value"].to(rt.color_description.second);
      rt.html_code.first = r["html_code"].to(rt.html_code.second);
      rt.distance.first = r["distance"].to(rt.distance.second);
      rt.ascent.first = r["ascent"].to(rt.ascent.second);
      rt.descent.first = r["descent"].to(rt.descent.second);
      rt.lowest.first = r["lowest"].to(rt.lowest.second);
      rt.highest.first = r["highest"].to(rt.highest.second);
    }
    if (!r["point_id"].is_null()) {
      route_point p;
      p.id.first = r["point_id"].to(p.id.second);
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      p.name.first = r["point_name"].to(p.name.second);
      p.comment.first = r["comment"].to(p.comment.second);
      p.description.first = r["description"].to(p.description.second);
      p.symbol.first = r["symbol"].to(p.symbol.second);
      p.altitude.first = r["altitude"].to(p.altitude.second);
      rt.points.push_back(p);
    }
  }
  if (rt.id.first)
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
      "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1"
      "WHERE (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
      "AND r.itinerary_id = $2 "
      "ORDER BY r.id, p.id",
      user_id,
      itinerary_id
    );
  route rt;
  for (const auto &r : result) {
    long route_id = r["route_id"].as<long>();
    if (!rt.id.first || rt.id.second != route_id) {
      if (rt.id.first) {
        routes.push_back(rt);
      }
      rt = route();
      rt.id.first = true;
      rt.id.second = route_id;
      rt.name.first = r["route_name"].to(rt.name.second);
      rt.color_key.first = r["path_color_key"].to(rt.color_key.second);
      rt.color_description.first = r["path_color_value"].to(rt.color_description.second);
      rt.html_code.first = r["html_code"].to(rt.html_code.second);
      rt.distance.first = r["distance"].to(rt.distance.second);
      rt.ascent.first = r["ascent"].to(rt.ascent.second);
      rt.descent.first = r["descent"].to(rt.descent.second);
      rt.lowest.first = r["lowest"].to(rt.lowest.second);
      rt.highest.first = r["highest"].to(rt.highest.second);
    }
    if (!r["point_id"].is_null()) {
      route_point p;
      p.id.first = r["point_id"].to(p.id.second);
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      p.name.first = r["point_name"].to(p.name.second);
      p.comment.first = r["comment"].to(p.comment.second);
      p.description.first = r["description"].to(p.description.second);
      p.symbol.first = r["symbol"].to(p.symbol.second);
      p.altitude.first = r["altitude"].to(p.altitude.second);
      rt.points.push_back(p);
    }
  }
  if (rt.id.first)
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
    "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1"
    "WHERE (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
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
    if (!trk.id.first || trk.id.second != track_id) {
      if (trk.id.first) {
        if (trkseg.id.first) {
          trk.segments.push_back(trkseg);
          trkseg = track_segment();
        }
        tracks.push_back(trk);
      }
      trk = track();
      trk.id.first = true;
      trk.id.second = track_id;
      trk.name.first = r["track_name"].to(trk.name.second);
      trk.distance.first = r["distance"].to(trk.distance.second);
      trk.ascent.first = r["ascent"].to(trk.ascent.second);
      trk.descent.first = r["descent"].to(trk.descent.second);
      trk.lowest.first = r["lowest"].to(trk.lowest.second);
      trk.highest.first = r["highest"].to(trk.highest.second);
      trk.color_key.first = r["path_color_key"].to(trk.color_key.second);
      trk.color_description.first =
        r["path_color_value"].to(trk.color_description.second);
      trk.html_code.first = r["html_code"].to(trk.html_code.second);
    }
    if (!r["segment_id"].is_null()) {
      long segment_id = r["segment_id"].as<long>();
      if (!trkseg.id.first || trkseg.id.second != segment_id) {
        if (trkseg.id.first) {
          trk.segments.push_back(trkseg);
          trkseg = track_segment();
        }
        trkseg.id.first = true;
        trkseg.id.second = segment_id;
        trkseg.distance.first = r["ts_distance"].to(trkseg.distance.second);
        trkseg.ascent.first = r["ts_ascent"].to(trkseg.ascent.second);
        trkseg.descent.first = r["ts_descent"].to(trkseg.descent.second);
        trkseg.lowest.first = r["ts_lowest"].to(trkseg.lowest.second);
        trkseg.highest.first = r["ts_highest"].to(trkseg.highest.second);
      }
      if (!r["point_id"].is_null()) {
        track_point p;
        p.id.first = r["point_id"].to(p.id.second);
        r["lng"].to(p.longitude);
        r["lat"].to(p.latitude);
        std::string s;
        if ((p.time.first = r["time"].to(s))) {
          DateTime time(s);
          p.time.second = time.time_tp();
        }
        p.hdop.first = r["hdop"].to(p.hdop.second);
        p.altitude.first = r["altitude"].to(p.altitude.second);
        trkseg.points.push_back(p);
      }
    }
  }
  if (trk.id.first) {
    if (trkseg.id.first)
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
    "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1"
    "WHERE (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
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
    if (!trk.id.first || trk.id.second != track_id) {
      if (trk.id.first) {
        if (trkseg.id.first) {
          trk.segments.push_back(trkseg);
          trkseg = track_segment();
        }
        tracks.push_back(trk);
      }
      trk = track();
      trk.id.first = true;
      trk.id.second = track_id;
      trk.name.first = r["track_name"].to(trk.name.second);
      trk.distance.first = r["distance"].to(trk.distance.second);
      trk.ascent.first = r["ascent"].to(trk.ascent.second);
      trk.descent.first = r["descent"].to(trk.descent.second);
      trk.lowest.first = r["lowest"].to(trk.lowest.second);
      trk.highest.first = r["highest"].to(trk.highest.second);
      trk.color_key.first = r["path_color_key"].to(trk.color_key.second);
      trk.color_description.first =
        r["path_color_value"].to(trk.color_description.second);
      trk.html_code.first = r["html_code"].to(trk.html_code.second);
    }
    if (!r["segment_id"].is_null()) {
      long segment_id = r["segment_id"].as<long>();
      if (!trkseg.id.first || trkseg.id.second != segment_id) {
        if (trkseg.id.first) {
          trk.segments.push_back(trkseg);
          trkseg = track_segment();
        }
        trkseg.id.first = true;
        trkseg.id.second = segment_id;
        trkseg.distance.first = r["ts_distance"].to(trkseg.distance.second);
        trkseg.ascent.first = r["ts_ascent"].to(trkseg.ascent.second);
        trkseg.descent.first = r["ts_descent"].to(trkseg.descent.second);
        trkseg.lowest.first = r["ts_lowest"].to(trkseg.lowest.second);
        trkseg.highest.first = r["ts_highest"].to(trkseg.highest.second);
        // std::cout << "Read segment ID: " << trkseg.id.second;
        // if (trkseg.distance.first)
        //   std::cout << " with distance " << trkseg.distance.second;
        // std::cout << '\n';
      }
      if (!r["point_id"].is_null()) {
        track_point p;
        p.id.first = r["point_id"].to(p.id.second);
        r["lng"].to(p.longitude);
        r["lat"].to(p.latitude);
        std::string s;
        if ((p.time.first = r["time"].to(s))) {
          DateTime time(s);
          p.time.second = time.time_tp();
        }
        p.hdop.first = r["hdop"].to(p.hdop.second);
        p.altitude.first = r["altitude"].to(p.altitude.second);
        trkseg.points.push_back(p);
      }
    }
  }
  if (trk.id.first) {
    if (trkseg.id.first)
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
      "w.time, w.altitude, w.symbol, w.comment, w.description, w.color, w.type, "
      "w.avg_samples "
      "FROM itinerary_waypoint w "
      "JOIN itinerary i ON i.id=w.itinerary_id "
      "LEFT JOIN itinerary_sharing sh "
      "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1"
      "WHERE  (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
      "AND w.itinerary_id=$2 AND w.id=ANY($3) "
      "ORDER BY name, symbol, id",
      user_id,
      itinerary_id,
      ids_sql);
  for (const auto &r : result) {
    waypoint wpt;
    wpt.id.first = r["id"].to(wpt.id.second);
    wpt.name.first = r["name"].to(wpt.name.second);
    r["lng"].to(wpt.longitude);
    r["lat"].to(wpt.latitude);
    std::string s;
    if ((wpt.time.first = r["time"].to(s))) {
      DateTime time(s);
      wpt.time.second = time.time_tp();
    }
    wpt.altitude.first = r["altitude"].to(wpt.altitude.second);
    wpt.symbol.first = r["symbol"].to(wpt.symbol.second);
    wpt.comment.first = r["comment"].to(wpt.comment.second);
    wpt.description.first = r["description"].to(wpt.description.second);
    wpt.color.first = r["color"].to(wpt.color.second);
    wpt.type.first = r["type"].to(wpt.type.second);
    wpt.avg_samples.first = r["avg_samples"].to(wpt.avg_samples.second);
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
      "w.time, w.altitude, w.symbol, w.comment, w.description, w.color, w.type, "
      "w.avg_samples "
      "FROM itinerary_waypoint w "
      "JOIN itinerary i ON i.id=w.itinerary_id "
      "LEFT JOIN itinerary_sharing sh "
      "ON sh.itinerary_id=$2 AND sh.shared_to_id=$1"
      "WHERE  (i.user_id=$1 OR (sh.active AND sh.shared_to_id=$1)) "
      "AND w.itinerary_id=$2"
      "ORDER BY name, symbol, id",
      user_id,
      itinerary_id);
  for (const auto &r : result) {
    waypoint wpt;
    wpt.id.first = r["id"].to(wpt.id.second);
    wpt.name.first = r["name"].to(wpt.name.second);
    r["lng"].to(wpt.longitude);
    r["lat"].to(wpt.latitude);
    std::string s;
    if ((wpt.time.first = r["time"].to(s))) {
      DateTime time(s);
      wpt.time.second = time.time_tp();
    }
    wpt.altitude.first = r["altitude"].to(wpt.altitude.second);
    wpt.symbol.first = r["symbol"].to(wpt.symbol.second);
    wpt.comment.first = r["comment"].to(wpt.comment.second);
    wpt.description.first = r["description"].to(wpt.description.second);
    wpt.color.first = r["color"].to(wpt.color.second);
    wpt.type.first = r["type"].to(wpt.type.second);
    wpt.avg_samples.first = r["avg_samples"].to(wpt.avg_samples.second);
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
    "color, type, avg_samples) "
    "VALUES ($1, $2, ST_SetSRID(ST_POINT($3, $4),4326), $5, $6, $7, $8, $9, "
    "$10, $11, $12)";
  connection->prepare(
      ps_name,
      sql);
  for (const auto &w : waypoints) {
    std::string timestr;
    if (w.time.first)
      timestr = DateTime(w.time.second).get_time_as_iso8601_gmt();
    tx.exec_prepared(
        ps_name,
        itinerary_id,
        w.name.first ? &w.name.second : nullptr,
        w.longitude,
        w.latitude,
        w.altitude.first ? &w.altitude.second : nullptr,
        w.time.first ? &timestr : nullptr,
        w.comment.first ? &w.comment.second : nullptr,
        w.description.first ? &w.description.second : nullptr,
        w.symbol.first ? &w.symbol.second : nullptr,
        w.color.first ? &w.color.second : nullptr,
        w.type.first ? &w.type.second : nullptr,
        w.avg_samples.first ? &w.avg_samples.second : nullptr);
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
        route.id.first ? &route.id.second : nullptr,
        p.longitude,
        p.latitude,
        p.altitude.first ? &p.altitude.second : nullptr,
        p.name.first ? &p.name.second : nullptr,
        p.comment.first ? &p.comment.second : nullptr,
        p.description.first ? &p.description.second : nullptr,
        p.symbol.first ? &p.symbol.second : nullptr);
    p.id.first = r["id"].to(p.id.second);
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
        r.name.first ? &r.name.second : nullptr,
        r.color_key.first ? &r.color_key.second : nullptr,
        r.distance.first ? &r.distance.second : nullptr,
        r.ascent.first ? &r.ascent.second : nullptr,
        r.descent.first ? &r.descent.second : nullptr,
        r.lowest.first ? &r.lowest.second : nullptr,
        r.highest.first ? &r.highest.second : nullptr);
    r.id.first = result["id"].to(r.id.second);
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
  int count = 0;
  for (auto &p : segment.points) {
    count++;
    std::string timestr;
    if (p.time.first)
      timestr = DateTime(p.time.second).get_time_as_iso8601_gmt();
    auto r = tx.exec_params1(
        sql,
        segment.id.first ? &segment.id.second : nullptr,
        p.longitude,
        p.latitude,
        p.time.first ? &timestr : nullptr,
        p.hdop.first ? &p.hdop.second : nullptr,
        p.altitude.first ? &p.altitude.second : nullptr);
    p.id.first = r["id"].to(p.id.second);
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
        track.id.first ? &track.id.second : nullptr,
        seg.distance.first ? &seg.distance.second : nullptr,
        seg.ascent.first ? &seg.ascent.second : nullptr,
        seg.descent.first ? &seg.descent.second : nullptr,
        seg.lowest.first ? &seg.lowest.second : nullptr,
        seg.highest.first ? &seg.highest.second : nullptr);
    seg.id.first = r["id"].to(seg.id.second);
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
        t.name.first ? &t.name.second : nullptr,
        t.color_key.first ? &t.color_key.second : nullptr,
        t.distance.first ? &t.distance.second : nullptr,
        t.ascent.first ? &t.ascent.second : nullptr,
        t.descent.first ? &t.descent.second : nullptr,
        t.lowest.first ? &t.lowest.second : nullptr,
        t.highest.first ? &t.highest.second : nullptr);
    t.id.first = r["id"].to(t.id.second);
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
 * \throws ItineraryPgDao::not_authorized if the user is not authorized.
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
 * \throws ItineraryPgDao::not_authorized if the user is not authorized.
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
    throw NotAuthorized();
}

/**
 * Validates that the specified user_id has write/update/delete access to the
 * specified itinerary.
 *
 * \param user_id the ID of the user to validate.
 * \param itinerary_id the ID of the itinerary to check.
 *
 * \throws ItineraryPgDao::not_authorized if the user is not authorized.
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
 * \throws ItineraryPgDao::not_authorized if the user is not authorized.
 */
void ItineraryPgDao::validate_user_itinerary_read_access(work &tx,
                                                           std::string user_id,
                                                           long itinerary_id)
{
  auto r = tx.exec_params1(
      "SELECT sum(count) FROM ("
      "SELECT COUNT(*) FROM itinerary WHERE user_id=$1 AND id=$2 "
      "UNION ALL "
      "SELECT COUNT (*) FROM itinerary_sharing "
      "WHERE active=true AND itinerary_id=$2 AND shared_to_id=$1) as q",
      user_id,
      itinerary_id
    );
  // If the user sets themselves as a shared user, the count is 2
  if (r[0].as<long>() < 1)
    throw NotAuthorized();
}

/**
 * Validates that the specified user_id has read access to the specified
 * itinerary.
 *
 * \param user_id the ID of the user to validate.
 * \param itinerary_id the ID of the itinerary to check.
 *
 * \throws ItineraryPgDao::not_authorized if the user is not authorized.
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
      "time, symbol, comment, description, color, type, avg_samples "
      "FROM itinerary_waypoint WHERE itinerary_id=$1 AND id=$2";
    row r = tx.exec_params1(sql,
                               itinerary_id,
                               waypoint_id);
    waypoint wpt;
    wpt.id.first = r["id"].to(wpt.id.second);
    wpt.name.first = r["name"].to(wpt.name.second);
    r["lng"].to(wpt.longitude);
    r["lat"].to(wpt.latitude);
    std::string s;
    if ((wpt.time.first = r["time"].to(s))) {
      DateTime time(s);
      wpt.time.second = time.time_tp();
    }
    wpt.altitude.first = r["altitude"].to(wpt.altitude.second);
    wpt.symbol.first = r["symbol"].to(wpt.symbol.second);
    wpt.comment.first = r["comment"].to(wpt.comment.second);
    wpt.description.first = r["description"].to(wpt.description.second);
    wpt.color.first = r["color"].to(wpt.color.second);
    wpt.type.first = r["type"].to(wpt.type.second);
    wpt.avg_samples.first = r["avg_samples"].to(wpt.avg_samples.second);
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

    std::string time_str;
    if (wpt.time.first)
      time_str = DateTime(wpt.time.second).get_time_as_iso8601_gmt();

    if (wpt.id.first) {
      tx.exec_params(
          "UPDATE itinerary_waypoint SET name=$3, "
          "geog=ST_SetSRID(ST_POINT($4, $5),4326), altitude=$6, time=$7, "
          "symbol=$8, comment=$9, description=$10, avg_samples=$11, type=$12, "
          "color=$13 WHERE itinerary_id=$1 AND id=$2",
          itinerary_id,
          wpt.id.second,
          wpt.name.first && !wpt.name.second.empty() ? &wpt.name.second : nullptr ,
          wpt.longitude,
          wpt.latitude,
          wpt.altitude.first ? &wpt.altitude.second : nullptr,
          time_str.empty() ? nullptr : &time_str,
          wpt.symbol.first && !wpt.symbol.second.empty() ? &wpt.symbol.second : nullptr,
          wpt.comment.first && !wpt.comment.second.empty() ? &wpt.comment.second : nullptr,
          wpt.description.first && !wpt.description.second.empty() ? &wpt.description.second : nullptr,
          wpt.avg_samples.first ? &wpt.avg_samples.second : nullptr,
          wpt.type.first && !wpt.type.second.empty() ? &wpt.type.second : nullptr,
          wpt.color.first && !wpt.color.second.empty() ? &wpt.color.second : nullptr);
    } else {
      auto r = tx.exec_params1(
          "INSERT INTO itinerary_waypoint (itinerary_id, name, geog, altitude, "
          "time, symbol, comment, description, avg_samples, type, color) "
          "VALUES ($1, $2, ST_SetSRID(ST_POINT($3, $4),4326), $5, $6, $7, $8, "
          "$9, $10, $11, $12) RETURNING id",
          itinerary_id,
          wpt.name.first && !wpt.name.second.empty() ? &wpt.name.second : nullptr ,
          wpt.longitude,
          wpt.latitude,
          wpt.altitude.first ? &wpt.altitude.second : nullptr,
          time_str.empty() ? nullptr : &time_str,
          wpt.symbol.first && !wpt.symbol.second.empty() ? &wpt.symbol.second : nullptr,
          wpt.comment.first && !wpt.comment.second.empty() ? &wpt.comment.second : nullptr,
          wpt.description.first && !wpt.description.second.empty() ? &wpt.description.second : nullptr,
          wpt.avg_samples.first ? &wpt.avg_samples.second : nullptr,
          wpt.type.first && !wpt.type.second.empty() ? &wpt.type.second : nullptr,
          wpt.color.first && !wpt.color.second.empty() ? &wpt.color.second : nullptr);
      wpt.id = std::make_pair(true, r["id"].as<long>());
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
    if (route.id.first) {
      // Validate the route belongs to this user
      auto check = tx.exec_params1(
          "SELECT COUNT(*) FROM itinerary_route "
          "WHERE itinerary_id=$1 AND id=$2",
          itinerary_id,
          route.id.second
        );
      if (check[0].as<long>() != 1)
        throw NotAuthorized();

      tx.exec_params("DELETE FROM itinerary_route_point "
                     "WHERE itinerary_route_id=$1",
                     route.id.second);
      auto r = tx.exec_params(
          "UPDATE itinerary_route "
          "SET name=$2, distance=$3, ascent=$4, descent=$5, lowest=$6, "
          "highest=$7, color=$8 "
          "WHERE id=$9 AND itinerary_id=$1",
          itinerary_id,
          route.name.first ? &route.name.second : nullptr,
          route.distance.first ? &route.distance.second : nullptr,
          route.ascent.first ? &route.ascent.second : nullptr,
          route.descent.first ? &route.descent.second : nullptr,
          route.lowest.first ? &route.lowest.second : nullptr,
          route.highest.first ? &route.highest.second : nullptr,
          route.color_key.first ? &route.color_key.second : nullptr,
          route.id.second
        );
    } else {
      auto r = tx.exec_params1(
          "INSERT INTO itinerary_route "
          "(itinerary_id, name, distance, ascent, descent, lowest, highest, "
          "color) "
          "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id",
          itinerary_id,
          route.name.first ? &route.name.second : nullptr,
          route.distance.first ? &route.distance.second : nullptr,
          route.ascent.first ? &route.ascent.second : nullptr,
          route.descent.first ? &route.descent.second : nullptr,
          route.lowest.first ? &route.lowest.second : nullptr,
          route.highest.first ? &route.highest.second : nullptr,
          route.color_key.first ? &route.color_key.second : nullptr
        );
      route.id = std::make_pair(true, r["id"].as<long>());
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
    if (track.id.first) {
      // Validate the track belongs to this user
      auto check = tx.exec_params1(
          "SELECT COUNT(*) FROM itinerary_track "
          "WHERE itinerary_id=$1 AND id=$2",
          itinerary_id,
          track.id.second
        );
      if (check[0].as<long>() != 1)
        throw NotAuthorized();

      tx.exec_params("DELETE FROM itinerary_track_segment "
                     "WHERE itinerary_track_id=$1",
                     track.id.second);
      tx.exec_params(
          "UPDATE itinerary_track "
          "SET name=$3, color=$4, distance=$5, ascent=$6, descent=$7, "
          "lowest=$8, highest=$9 "
          "WHERE id=$2 AND itinerary_id=$1",
          itinerary_id,
          track.id.second,
          track.name.first ? &track.name.second : nullptr,
          track.color_key.first ? &track.color_key.second : nullptr,
          track.distance.first ? &track.distance.second : nullptr,
          track.ascent.first ? &track.ascent.second : nullptr,
          track.descent.first ? &track.descent.second : nullptr,
          track.lowest.first ? &track.lowest.second : nullptr,
          track.highest.first ? &track.highest.second : nullptr
        );
    } else {
      auto r = tx.exec_params1(
          "INSERT INTO itinerary_track "
          "(itinerary_id, name, color, distance, ascent, descent, lowest, highest) "
          "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id",
          itinerary_id,
          track.name.first ? &track.name.second : nullptr,
          track.color_key.first ? &track.color_key.second : nullptr,
          track.distance.first ? &track.distance.second : nullptr,
          track.ascent.first ? &track.ascent.second : nullptr,
          track.descent.first ? &track.descent.second : nullptr,
          track.lowest.first ? &track.lowest.second : nullptr,
          track.highest.first ? &track.highest.second : nullptr
        );
      track.id = std::make_pair(true, r["id"].as<long>());
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
                                         std::string user_id,
                                         long itinerary_id,
                                         std::uint32_t offset,
                                         int limit)
{
  try {
    auto result = tx.exec_params(
        "SELECT sh.shared_to_id, u.nickname, sh.active "
        "FROM itinerary_sharing sh "
        "JOIN usertable u ON u.id=sh.shared_to_id "
        "WHERE sh.itinerary_id=$1 "
        "ORDER BY u.nickname "
        "OFFSET $2 LIMIT $3",
        itinerary_id,
        offset,
        limit < 0 ? nullptr : &limit);
    std::vector<itinerary_share> shares;
    for (const auto &r : result) {
      itinerary_share share;
      share.shared_to_id = r["shared_to_id"].as<long>();
      share.nickname = r["nickname"].as<std::string>();
      share.active.first = r["active"].to(share.active.second);
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
  auto shares = get_itinerary_shares(tx, user_id, itinerary_id, offset, limit);
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
    share.active.first = r["active"].to(share.active.second);
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
        share.active.first ? &share.active.second : nullptr
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
          share.active.first ? &share.active.second : nullptr
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
  if (rhs.time.first) {
    DateTime dt(rhs.time.second);
    node["time"] = dt.get_time_as_iso8601_gmt();
  } else {
    node["time"] = YAML::Null;
  }
  if (rhs.hdop.first)
    node["hdop"] = rhs.hdop.second;
  else
    node["hdop"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::track_point::decode(
    const YAML::Node& node, ItineraryPgDao::track_point& rhs)
{
  if ((rhs.time.first = node["time"] && !node["time"].IsNull())) {
    DateTime time(node["time"].as<std::string>());
    rhs.time.second = time.time_tp();
  }
  if ((rhs.hdop.first = node["hdop"] && !node["hdop"].IsNull()))
    rhs.hdop.second = node["hdop"].as<float>();
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
  if (rhs.name.first)
    node["name"] = rhs.name.second;
  else
    node["name"] = YAML::Null;
  if (rhs.comment.first)
    node["comment"] = rhs.comment.second;
  else
    node["comment"] = YAML::Null;
  if (rhs.symbol.first)
    node["symbol"] = rhs.symbol.second;
  else
    node["symbol"] = YAML::Null;
  if (rhs.type.first)
    node["type"] = rhs.type.second;
  else
    node["type"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::waypoint_base::decode(
    const YAML::Node& node, ItineraryPgDao::waypoint_base& rhs)
{
  if ((rhs.name.first = node["name"] && !node["name"].IsNull()))
    rhs.name.second = node["name"].as<std::string>();
  if ((rhs.comment.first = node["comment"] && !node["comment"].IsNull()))
    rhs.comment.second = node["comment"].as<std::string>();
  if ((rhs.symbol.first = node["symbol"] && !node["symbol"].IsNull()))
    rhs.symbol.second = node["symbol"].as<std::string>();
  if ((rhs.type.first = node["type"] && !node["type"].IsNull()))
    rhs.type.second = node["type"].as<std::string>();
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

  if (rhs.time.first) {
    DateTime dt(rhs.time.second);
    node["time"] = dt.get_time_as_iso8601_gmt();
  } else {
    node["time"] = YAML::Null;
  }
  if (rhs.description.first)
    node["description"] = rhs.description.second;
  else
    node["description"] = YAML::Null;
  if (rhs.color.first)
    node["color"] = rhs.color.second;
  else
    node["color"] = YAML::Null;
  if (rhs.type.first)
    node["type"] = rhs.type.second;
  else
    node["type"] = YAML::Null;
  if (rhs.avg_samples.first)
    node["samples"] = rhs.avg_samples.second;
  else
    node["samples"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::waypoint::decode(
    const YAML::Node& node, ItineraryPgDao::waypoint& rhs)
{
  if ((rhs.time.first = node["time"] && !node["time"].IsNull())) {
    DateTime time(node["time"].as<std::string>());
    rhs.time.second = time.time_tp();
  }
  if ((rhs.description.first = node["description"] &&
       !node["description"].IsNull()))
    rhs.description.second = node["description"].as<std::string>();
  if ((rhs.color.first = node["color"] && !node["color"].IsNull()))
    rhs.color.second = node["color"].as<std::string>();
  if ((rhs.type.first = node["type"] && !node["type"].IsNull()))
    rhs.type.second = node["type"].as<std::string>();
  if ((rhs.avg_samples.first = node["samples"] && !node["samples"].IsNull()))
    rhs.avg_samples.second = node["samples"].as<long>();
  auto success = ItineraryPgDao::waypoint_base::decode(node, rhs);
  return location::decode(node, rhs) && success;
}

YAML::Node ItineraryPgDao::itinerary_share::encode(
    const ItineraryPgDao::itinerary_share& rhs)
{
  YAML::Node node;
  node["nickname"] = rhs.nickname;
  if (rhs.active.first)
    node["active"] = rhs.active.second;
  else
    node["active"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::itinerary_share::decode(
    const YAML::Node& node, ItineraryPgDao::itinerary_share& rhs)
{
  rhs.nickname = node["nickname"].as<std::string>();
  if ((rhs.active.first = node["active"] && !node["active"].IsNull())) {
    rhs.active.second = node["active"].as<bool>();
  }
  return true;
}

YAML::Node ItineraryPgDao::itinerary_base::encode(
    const ItineraryPgDao::itinerary_base& rhs)
{
  YAML::Node node;
  if (rhs.id.first)
    node["id"] = rhs.id.second;
  else
    node["id"] = YAML::Null;
  if (rhs.start.first) {
    DateTime start(rhs.start.second);
    node["start"] = start.get_time_as_iso8601_gmt();
  } else {
    node["start"] = YAML::Null;
  }
  if (rhs.finish.first) {
    DateTime finish(rhs.finish.second);
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
  if ((rhs.id.first = node["id"] && !node["id"].IsNull()))
    rhs.id.second = node["id"].as<long>();
  if ((rhs.start.first = node["start"] && !node["start"].IsNull())) {
    DateTime start(node["start"].as<std::string>());
    rhs.start.second = start.time_tp();
  }
  if ((rhs.finish.first = node["finish"] && !node["finish"].IsNull())) {
    DateTime finish(node["finish"].as<std::string>());
    rhs.finish.second = finish.time_tp();
  }
  rhs.title = node["title"].as<std::string>();
  return true;
}

YAML::Node ItineraryPgDao::itinerary_summary::encode(
    const ItineraryPgDao::itinerary_summary& rhs)
{
  auto node = ItineraryPgDao::itinerary_base::encode(rhs);
  if (rhs.owner_nickname.first)
    node["owned_by_nickname"] = rhs.owner_nickname.second;
  else
    node["owned_by_nickname"] = YAML::Null;
  if (rhs.shared.first)
    node["shared"] = rhs.shared.second;
  else
    node["shared"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::itinerary_summary::decode(
    const YAML::Node& node, ItineraryPgDao::itinerary_summary& rhs)
{
  if ((rhs.owner_nickname.first = node["owned_by_nickname"] &&
       !node["owned_by_nickname"].IsNull()))
    rhs.owner_nickname.second = node["owned_by_nickname"].as<std::string>();
  if ((rhs.shared.first = node["shared"] && !node["shared"].IsNull()))
    rhs.shared.second = node["shared"].as<bool>();
  return ItineraryPgDao::itinerary_base::decode(node, rhs);
}

YAML::Node ItineraryPgDao::itinerary_description::encode(
    const ItineraryPgDao::itinerary_description& rhs)
{
  auto node = ItineraryPgDao::itinerary_summary::encode(rhs);
  if (rhs.description.first)
    node["description"] = rhs.description.second;
  else
    node["description"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::itinerary_description::decode(
    const YAML::Node& node, ItineraryPgDao::itinerary_description& rhs)
{
  if ((rhs.description.first = node["description"] &&
       !node["description"].IsNull()))
    rhs.description.second = node["description"].as<std::string>();
  return ItineraryPgDao::itinerary_summary::decode(node, rhs);
}

YAML::Node ItineraryPgDao::itinerary_detail::encode(
    const ItineraryPgDao::itinerary_detail& rhs)
{
  auto node = ItineraryPgDao::itinerary_description::encode(rhs);
  if (rhs.shared_to_nickname.first)
    node["shared_to_nickname"] = rhs.shared_to_nickname.second;
  else
    node["shared_to_nickname"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::itinerary_detail::decode(
    const YAML::Node& node, ItineraryPgDao::itinerary_detail& rhs)
{
  if ((rhs.shared_to_nickname.first = node["shared_to_nickname"] &&
       !node["shared_to_nickname"].IsNull()))
    rhs.shared_to_nickname.second = node["shard_to_nickname"].as<std::string>();
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
  if (rhs.name.first)
    node["name"] = rhs.name.second;
  else
    node["name"] = YAML::Null;
  if (rhs.comment.first)
    node["comment"] = rhs.comment.second;
  else
    node["comment"] = YAML::Null;
  if (rhs.description.first)
    node["description"] = rhs.description.second;
  else
    node["description"] = YAML::Null;
  if (rhs.symbol.first)
    node["symbol"] = rhs.symbol.second;
  else
    node["symbol"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::route_point::decode(
    const YAML::Node& node, ItineraryPgDao::route_point& rhs)
{
  if ((rhs.name.first = node["name"] && !node["name"].IsNull()))
    rhs.name.second = node["name"].as<std::string>();
  if ((rhs.comment.first = node["comment"] && !node["comment"].IsNull()))
    rhs.comment.second = node["comment"].as<std::string>();
  if ((rhs.description.first = node["description"] && !node["description"].IsNull()))
    rhs.description.second = node["description"].as<std::string>();
  if ((rhs.symbol.first = node["symbol"] && !node["symbol"].IsNull()))
    rhs.symbol.second = node["symbol"].as<std::string>();
  return location::decode(node, rhs);
}

YAML::Node
    ItineraryPgDao::path_base::encode(const ItineraryPgDao::path_base& rhs)
{
  YAML::Node node = path_statistics::encode(rhs);
  if (rhs.id.first)
    node["id"] = rhs.id.second;
  else
    node["id"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::path_base::decode(
    const YAML::Node& node, ItineraryPgDao::path_base& rhs)
{
  if ((rhs.id.first = node["id"] && !node["id"].IsNull()))
    rhs.id.second = node["id"].as<long>();
  return path_statistics::decode(node, rhs);
}

YAML::Node
    ItineraryPgDao::path_summary::encode(const ItineraryPgDao::path_summary& rhs)
{
  YAML::Node node = ItineraryPgDao::path_base::encode(rhs);
  if (rhs.name.first)
    node["name"] = rhs.name.second;
  else
    node["name"] = YAML::Null;
  if (rhs.color_key.first)
    node["color"] = rhs.color_key.second;
  else
    node["color"] = YAML::Null;
  if (rhs.html_code.first)
    node["htmlcolor"] = rhs.html_code.second;
  else
    node["htmlcolor"] = YAML::Null;
  return node;
}

bool ItineraryPgDao::path_summary::decode(const YAML::Node& node,
                                          ItineraryPgDao::path_summary& rhs)
{
  if ((rhs.name.first = node["name"] && !node["name"].IsNull()))
    rhs.name.second = node["name"].as<std::string>();
  if ((rhs.color_key.first = node["color"] && !node["color"].IsNull()))
    rhs.color_key.second = node["color"].as<std::string>();
  if ((rhs.html_code.first = node["htmlcolor"] && !node["htmlcolor"].IsNull()))
    rhs.html_code.second = node["htmlcolor"].as<std::string>();
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
    route.id.first = rt["id"].to(route.id.second);
    route.name.first = rt["name"].to(route.name.second);
    route.color_key.first = rt["color"].to(route.color_key.second);
    route.distance.first = rt["distance"].to(route.distance.second);
    route.ascent.first = rt["ascent"].to(route.ascent.second);
    route.descent.first = rt["descent"].to(route.descent.second);
    route.lowest.first = rt["lowest"].to(route.lowest.second);
    route.highest.first = rt["highest"].to(route.highest.second);
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
      route.id.first = rt["id"].to(route.id.second);
      route.name.first = rt["name"].to(route.name.second);
      route.color_key.first = rt["color"].to(route.color_key.second);
      route.html_code.first = rt["html_code"].to(route.html_code.second);
      route.color_description.first =
        rt["path_color_value"].to(route.color_description.second);
      route.distance.first = rt["distance"].to(route.distance.second);
      route.ascent.first = rt["ascent"].to(route.ascent.second);
      route.descent.first = rt["descent"].to(route.descent.second);
      route.lowest.first = rt["lowest"].to(route.lowest.second);
      route.highest.first = rt["highest"].to(route.highest.second);
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
    track.id.first = rt["id"].to(track.id.second);
    track.name.first = rt["name"].to(track.name.second);
    track.color_key.first = rt["color"].to(track.color_key.second);
    track.html_code.first = rt["html_code"].to(track.html_code.second);
    track.distance.first = rt["distance"].to(track.distance.second);
    track.ascent.first = rt["ascent"].to(track.ascent.second);
    track.descent.first = rt["descent"].to(track.descent.second);
    track.lowest.first = rt["lowest"].to(track.lowest.second);
    track.highest.first = rt["highest"].to(track.highest.second);
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
      track.id.first = trk["id"].to(track.id.second);
      track.name.first = trk["name"].to(track.name.second);
      track.color_key.first = trk["color"].to(track.color_key.second);
      track.html_code.first = trk["html_code"].to(track.html_code.second);
      track.color_description.first =
        trk["path_color_value"].to(track.color_description.second);
      track.distance.first = trk["distance"].to(track.distance.second);
      track.ascent.first = trk["ascent"].to(track.ascent.second);
      track.descent.first = trk["descent"].to(track.descent.second);
      track.lowest.first = trk["lowest"].to(track.lowest.second);
      track.highest.first = trk["highest"].to(track.highest.second);
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
  if (!route.id.first)
    throw std::invalid_argument("Route ID not set");
  try {
    work tx(*connection);
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    tx.exec_params(
        "UPDATE itinerary_route "
        "SET name=$3, color=$4 "
        "WHERE id=$2 AND itinerary_id=$1",
        itinerary_id,
        route.id.second,
        route.name.first ? &route.name.second : nullptr,
        route.color_key.first ? &route.color_key.second : nullptr
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
  if (!track.id.first)
    throw std::invalid_argument("Track ID not set");
  try {
    work tx(*connection);
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    tx.exec_params(
        "UPDATE itinerary_track "
        "SET name=$3, color=$4 "
        "WHERE id=$2 AND itinerary_id=$1",
        itinerary_id,
        track.id.second,
        track.name.first ? &track.name.second : nullptr,
        track.color_key.first ? &track.color_key.second : nullptr
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
    track.id.first = r["id"].to(track.id.second);
    track.name.first = r["name"].to(track.name.second);
    track.color_key.first = r["color_key"].to(track.color_key.second);
    track.color_description.first =
      r["color_description"].to(track.color_description.second);
    track.html_code.first = r["html_code"].to(track.html_code.second);
    track.distance.first = r["distance"].to(track.distance.second);
    track.ascent.first = r["ascent"].to(track.ascent.second);
    track.descent.first = r["descent"].to(track.descent.second);
    track.lowest.first = r["lowest"].to(track.lowest.second);
    track.highest.first = r["highest"].to(track.highest.second);
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
      trkseg.id.first = r["id"].to(trkseg.id.second);
      trkseg.distance.first = r["distance"].to(trkseg.distance.second);
      trkseg.ascent.first = r["ascent"].to(trkseg.ascent.second);
      trkseg.descent.first = r["descent"].to(trkseg.descent.second);
      trkseg.lowest.first = r["lowest"].to(trkseg.lowest.second);
      trkseg.highest.first = r["highest"].to(trkseg.highest.second);
      if (trkseg.id.first)
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
      p.id.first = r["id"].to(p.id.second);
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      std::string s;
      if ((p.time.first = r["time"].to(s))) {
        DateTime time(s);
        p.time.second = time.time_tp();
      }
      p.hdop.first = r["hdop"].to(p.hdop.second);
      p.altitude.first = r["altitude"].to(p.altitude.second);
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
      p.id.first = r["id"].to(p.id.second);
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      p.altitude.first = r["altitude"].to(p.altitude.second);
      // p.name.first = r["name"].to(p.name.second);
      // p.comment.first = r["comment"].to(p.comment.second);
      // p.description.first = r["description"].to(p.description.second);
      // p.symbol.first = r["symbol"].to.p.symbol.second);
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
    route.id.first = true;
    route.id.second = route_id;
    route.name.first = r["route_name"].to(route.name.second);
    route.color_key.first = r["path_color"].to(route.color_key.second);
    route.color_description.first =
      r["path_color_value"].to(route.color_description.second);
    route.html_code.first = r["html_code"].to(route.html_code.second);
    route.distance.first = r["distance"].to(route.distance.second);
    route.ascent.first = r["ascent"].to(route.ascent.second);
    route.descent.first = r["descent"].to(route.descent.second);
    route.lowest.first = r["lowest"].to(route.lowest.second);
    route.highest.first = r["highest"].to(route.highest.second);

    auto result = tx.exec_params(
        "SELECT p.id AS point_id, "
        "ST_X(p.geog::geometry) as lng, ST_Y(p.geog::geometry) as lat, "
        "p.name AS point_name, p.comment, p.description, p.symbol, p.altitude "
        "FROM itinerary_route_point p "
        "WHERE p.itinerary_route_id=$1 "
        "ORDER BY p.id OFFSET $2 LIMIT $3",
        route_id,
        offset,
        limit < 0 ? nullptr : &limit);

    for (const auto &r : result) {
      route_point p;
      p.id.first = r["point_id"].to(p.id.second);
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      p.name.first = r["point_name"].to(p.name.second);
      p.comment.first = r["comment"].to(p.comment.second);
      p.description.first = r["description"].to(p.description.second);
      p.symbol.first = r["symbol"].to(p.symbol.second);
      p.altitude.first = r["altitude"].to(p.altitude.second);
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
    trkseg.id.first = r["id"].to(trkseg.id.second);
    trkseg.distance.first = r["distance"].to(trkseg.distance.second);
    trkseg.ascent.first = r["ascent"].to(trkseg.ascent.second);
    trkseg.descent.first = r["descent"].to(trkseg.descent.second);
    trkseg.lowest.first = r["lowest"].to(trkseg.lowest.second);
    trkseg.highest.first = r["highest"].to(trkseg.highest.second);
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
        limit < 0 ? nullptr : &limit
      );
    for (const auto &r : result) {
      track_point p;
      p.id.first = r["id"].to(p.id.second);
      r["lng"].to(p.longitude);
      r["lat"].to(p.latitude);
      std::string s;
      if ((p.time.first = r["time"].to(s))) {
        DateTime time(s);
        p.time.second = time.time_tp();
      }
      p.hdop.first = r["hdop"].to(p.hdop.second);
      p.altitude.first = r["altitude"].to(p.altitude.second);
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
      trkseg.id.first = seg["id"].to(trkseg.id.second);
      if (trkseg.id.first) {
        trkseg.distance.first = seg["distance"].to(trkseg.distance.second);
        trkseg.ascent.first = seg["ascent"].to(trkseg.ascent.second);
        trkseg.descent.first = seg["descent"].to(trkseg.descent.second);
        trkseg.lowest.first = seg["lowest"].to(trkseg.lowest.second);
        trkseg.highest.first = seg["highest"].to(trkseg.highest.second);
        auto result = tx.exec_prepared(
            "fetch-segment-points",
            itinerary_id,
            trkseg.id.second
          );
        for (const auto &r : result) {
          track_point p;
          p.id.first = r["id"].to(p.id.second);
          r["lng"].to(p.longitude);
          r["lat"].to(p.latitude);
          std::string s;
          if ((p.time.first = r["time"].to(s))) {
            DateTime time(s);
            p.time.second = time.time_tp();
          }
          p.hdop.first = r["hdop"].to(p.hdop.second);
          p.altitude.first = r["altitude"].to(p.altitude.second);
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
  if (!track.id.first)
    throw std::invalid_argument("Track ID not set");
  try {
    work tx(*connection);
    validate_user_itinerary_modification_access(tx, user_id, itinerary_id);
    tx.exec_params(
        "UPDATE itinerary_track "
        "SET distance=$3, ascent=$4, descent=$5, lowest=$6, highest=$7 "
        "WHERE id=$2 AND itinerary_id=$1",
        itinerary_id,
        track.id.second,
        track.distance.first ? &track.distance.second : nullptr,
        track.ascent.first ? &track.ascent.second : nullptr,
        track.descent.first ? &track.descent.second : nullptr,
        track.lowest.first ? &track.lowest.second : nullptr,
        track.highest.first ? &track.highest.second : nullptr
      );
    const std::string ps_name = "update_track_segment_stats";
    connection->prepare(
        ps_name,
        "UPDATE itinerary_track_segment "
        "SET distance=$3, ascent=$4, descent=$5, lowest=$6, highest=$7 "
        "WHERE id=$2 AND itinerary_track_id=$1"
      );
    for (const auto &ts : track.segments) {
      if (!ts.id.first)
        throw std::invalid_argument("Track segment ID not set");
      tx.exec_prepared(
          ps_name,
          track.id.second,
          ts.id.second,
          ts.distance.first ? &ts.distance.second : nullptr,
          ts.ascent.first ? &ts.ascent.second : nullptr,
          ts.descent.first ? &ts.descent.second : nullptr,
          ts.lowest.first ? &ts.lowest.second : nullptr,
          ts.highest.first ? &ts.highest.second : nullptr
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
      it.id.first = r["id"].to(it.id.second);
      std::string s;
      if ((it.start.first = r["start"].to(s))) {
        DateTime start(s);
        it.start.second = start.time_tp();
      }
      if ((it.finish.first = r["finish"].to(s))) {
        DateTime finish(s);
        it.finish.second = finish.time_tp();
      }
      r["title"].to(it.title);
      it.owner_nickname.first = r["nickname"].to(it.owner_nickname.second);
      it.shared.first = it.owner_nickname.first && !it.owner_nickname.second.empty();
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
