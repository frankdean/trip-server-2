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
#include <memory>
#include <vector>

using namespace fdsd::trip;
using namespace fdsd::utils;
using namespace pqxx;


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

std::pair<bool, ItineraryPgDao::itinerary> ItineraryPgDao::get_itinerary_details(
    std::string user_id, long itinerary_id)
{
  try {
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
    work tx(*connection);
    auto result = tx.exec_params(
        sql,
        user_id,
        itinerary_id
      );
    ItineraryPgDao::itinerary it;
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
    it.shared_to_nickname.first = r["shared_to_nickname"].to(it.shared_to_nickname.second);
    it.shared.first = (it.shared.second = it.shared_to_nickname.first);
    it.description.first = r["description"].to(it.description.second);
    // Query routes, tracks and waypoints
    auto route_result = tx.exec_params(
        "SELECT id, name, color, distance, ascent, descent, lowest, highest "
        "FROM itinerary_route WHERE itinerary_id=$1 ORDER BY name, id",
        itinerary_id);
    for (const auto &rt : route_result) {
      path_summary route;
      route.id.first = rt["id"].to(route.id.second);
      route.name.first = rt["name"].to(route.name.second);
      route.color.first = rt["color"].to(route.color.second);
      route.distance.first = rt["distance"].to(route.distance.second);
      route.ascent.first = rt["ascent"].to(route.ascent.second);
      route.descent.first = rt["descent"].to(route.descent.second);
      route.lowest.first = rt["lowest"].to(route.lowest.second);
      route.highest.first = rt["highest"].to(route.highest.second);
      it.routes.push_back(std::make_shared<path_summary>(route));
    }
    auto track_result = tx.exec_params(
        "SELECT id, name, color, distance, ascent, descent, lowest, highest "
        "FROM itinerary_track WHERE itinerary_id=$1 ORDER BY name, id",
        itinerary_id);
    for (const auto &tk : track_result) {
      path_summary track;
      track.id.first = tk["id"].to(track.id.second);
      track.name.first = tk["name"].to(track.name.second);
      track.color.first = tk["color"].to(track.color.second);
      track.distance.first = tk["distance"].to(track.distance.second);
      track.ascent.first = tk["ascent"].to(track.ascent.second);
      track.descent.first = tk["descent"].to(track.descent.second);
      track.lowest.first = tk["lowest"].to(track.lowest.second);
      track.highest.first = tk["highest"].to(track.highest.second);
      it.tracks.push_back(std::make_shared<path_summary>(track));
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
      it.waypoints.push_back(std::make_shared<waypoint_summary>(waypoint));
    }

    tx.commit();
    return std::make_pair(true, it);
  } catch (const std::exception &e) {
    std::cerr << "Exception executing query to fetch itinerary details: "
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

void ItineraryPgDao::save_itinerary(
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
    if (itinerary.id.first) {
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
      tx.exec_params(
          "INSERT INTO itinerary "
          "(user_id, title, start, finish, description) "
          "VALUES ($1, $2, $3, $4, $5)",
          user_id,
          itinerary.title,
          itinerary.start.first ? &start : nullptr,
          itinerary.finish.first ? &finish : nullptr,
          itinerary.description.first ? &itinerary.description.second : nullptr
        );
    }
    tx.commit();
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

std::vector<std::unique_ptr<ItineraryPgDao::route>>
    ItineraryPgDao::get_routes(std::string user_id,
                               long itinerary_id,
                               std::vector<long> route_ids)
{
  std::vector<std::unique_ptr<route>> routes;
  if (route_ids.empty())
    return routes;
  work tx(*connection);
  std::string route_ids_sql = dao_helper::to_sql_array(route_ids);
  auto result = tx.exec_params(
      "SELECT r.id AS route_id, r.name AS route_name, r.color AS path_color, "
      "r.distance, r.ascent, r.descent, r.lowest, r.highest, "
      "p.id AS point_id, "
      "ST_X(p.geog::geometry) as lng, ST_Y(p.geog::geometry) as lat, "
      "p.name AS point_name, p.comment, p.description, p.symbol, p.altitude "
      "FROM itinerary_route r "
      "JOIN itinerary i ON i.id=r.itinerary_id "
      "LEFT JOIN path_color rc ON r.color=rc.key "
      "LEFT JOIN itinerary_route_point p ON r.id=p.itinerary_route_id "
      "WHERE i.user_id=$1 AND r.itinerary_id = $2 AND r.id=ANY($3) "
      "ORDER BY r.id, p.id",
      user_id,
      itinerary_id,
      route_ids_sql
    );
  std::unique_ptr<route> rt;
  for (const auto &r : result) {
    long route_id = r["route_id"].as<long>();
    if (!rt || rt->id.second != route_id) {
      if (rt)
        routes.push_back(std::move(rt));
      rt = std::unique_ptr<route>(new route);
      rt->id.first = true;
      rt->id.second = route_id;
      rt->name.first = r["route_name"].to(rt->name.second);
      rt->color.first = r["path_color"].to(rt->color.second);
      rt->distance.first = r["distance"].to(rt->distance.second);
      rt->ascent.first = r["ascent"].to(rt->ascent.second);
      rt->descent.first = r["descent"].to(rt->descent.second);
      rt->lowest.first = r["lowest"].to(rt->lowest.second);
      rt->highest.first = r["highest"].to(rt->highest.second);
    }
    if (!r["point_id"].is_null()) {
      std::unique_ptr<route_point> p(new route_point);
      p->id.first = r["point_id"].to(p->id.second);
      r["lng"].to(p->longitude);
      r["lat"].to(p->latitude);
      p->name.first = r["point_name"].to(p->name.second);
      p->comment.first = r["comment"].to(p->comment.second);
      p->description.first = r["description"].to(p->description.second);
      p->symbol.first = r["symbol"].to(p->symbol.second);
      p->altitude.first = r["altitude"].to(p->altitude.second);
      rt->points.push_back(std::move(p));
    }
  }
  if (rt)
    routes.push_back(std::move(rt));
  tx.commit();
  return routes;
}

std::vector<std::unique_ptr<ItineraryPgDao::track>>
    ItineraryPgDao::get_tracks(std::string user_id,
                               long itinerary_id,
                               std::vector<long> ids)
{
  std::vector<std::unique_ptr<track>> tracks;
  if (ids.empty())
    return tracks;
  work tx(*connection);
  std::string ids_sql = dao_helper::to_sql_array(ids);
  const std::string sql =
    "SELECT t.id AS track_id, t.name AS track_name, t.color AS path_color, "
    "t.distance, t.ascent, t.descent, t.lowest, t.highest, "
    "ts.id AS segment_id, p.id AS point_id, "
    "ST_X(p.geog::geometry) as lng, ST_Y(p.geog::geometry) as lat, "
    "p.time, p.hdop, p.altitude "
    "FROM itinerary_track t "
    "JOIN itinerary i ON i.id=t.itinerary_id "
    "LEFT JOIN itinerary_track_segment ts ON t.id=ts.itinerary_track_id "
    "LEFT JOIN itinerary_track_point p ON ts.id=p.itinerary_track_segment_id "
    "WHERE i.user_id=$1 AND t.itinerary_id=$2 AND t.id=ANY($3) ORDER BY t.id, ts.id, p.id";
  // std::cout << "SQL: \n" << sql << '\n' << "track ids: " << ids_sql << '\n';
  auto result = tx.exec_params(
      sql,
      user_id,
      itinerary_id,
      ids_sql
    );
  // std::cout << "Got " << result.size() << " track points for itinerary " << itinerary_id << "\n";
  std::unique_ptr<track> trk;
  std::unique_ptr<track_segment> trkseg;
  for (const auto &r : result) {
    long track_id = r["track_id"].as<long>();
    if (!trk || trk->id.second != track_id) {
      if (trk) {
        if (trkseg)
          trk->segments.push_back(std::move(trkseg));
        tracks.push_back(std::move(trk));
      }
      trk = std::unique_ptr<track>(new track);
      trk->id.first = true;
      trk->id.second = track_id;
      trk->name.first = r["track_name"].to(trk->name.second);
      trk->distance.first = r["distance"].to(trk->distance.second);
      trk->ascent.first = r["ascent"].to(trk->ascent.second);
      trk->descent.first = r["descent"].to(trk->descent.second);
      trk->lowest.first = r["lowest"].to(trk->lowest.second);
      trk->highest.first = r["highest"].to(trk->highest.second);
      trk->color.first = r["path_color"].to(trk->color.second);
    }
    if (!r["segment_id"].is_null()) {
      long segment_id = r["segment_id"].as<long>();
      if (!trkseg || trkseg->id.second != segment_id) {
        if (trkseg)
          trk->segments.push_back(std::move(trkseg));
        trkseg = std::unique_ptr<track_segment>(new track_segment);
        trkseg->id.first = true;
        trkseg->id.second = segment_id;
        trkseg->distance.first = r["distance"].to(trkseg->distance.second);
        trkseg->ascent.first = r["ascent"].to(trkseg->ascent.second);
        trkseg->descent.first = r["descent"].to(trkseg->descent.second);
        trkseg->lowest.first = r["lowest"].to(trkseg->lowest.second);
        trkseg->highest.first = r["highest"].to(trkseg->highest.second);
      }
      if (!r["point_id"].is_null()) {
        std::unique_ptr<track_point> p(new track_point);
        p->id.first = r["point_id"].to(p->id.second);
        r["lng"].to(p->longitude);
        r["lat"].to(p->latitude);
        std::string s;
        if ((p->time.first = r["time"].to(s))) {
          DateTime time(s);
          p->time.second = time.time_tp();
        }
        p->hdop.first = r["hdop"].to(p->hdop.second);
        p->altitude.first = r["altitude"].to(p->altitude.second);
        trkseg->points.push_back(std::move(p));
      }
    }
  }
  if (trk && trkseg)
    trk->segments.push_back(std::move(trkseg));
  if (trk)
    tracks.push_back(std::move(trk));
  tx.commit();
  return tracks;
}

std::vector<std::unique_ptr<ItineraryPgDao::waypoint>>
    ItineraryPgDao::get_waypoints(std::string user_id,
                                  long itinerary_id,
                                  std::vector<long> ids)
{
  std::vector<std::unique_ptr<waypoint>> waypoints;
  if (ids.empty())
    return waypoints;
  work tx(*connection);
  std::string ids_sql = dao_helper::to_sql_array(ids);
  auto result = tx.exec_params(
      "SELECT w.id, w.name, "
      "ST_X(geog::geometry) as lng, ST_Y(geog::geometry) as lat, "
      "w.time, w.altitude, w.symbol, w.comment, w.description, w.color, w.type, "
      "w.avg_samples "
      "FROM itinerary_waypoint w "
      "JOIN itinerary i ON i.id=w.itinerary_id "
      "WHERE  i.user_id=$1 AND w.itinerary_id=$2 AND w.id=ANY($3)"
      "ORDER BY name, symbol, id",
      user_id,
      itinerary_id,
      ids_sql);
  for (const auto &r : result) {
    std::unique_ptr<waypoint> wpt(new waypoint);
    wpt->id.first = r["id"].to(wpt->id.second);
    wpt->name.first = r["name"].to(wpt->name.second);
    r["lng"].to(wpt->longitude);
    r["lat"].to(wpt->latitude);
    std::string s;
    if ((wpt->time.first = r["time"].to(s))) {
      DateTime time(s);
      wpt->time.second = time.time_tp();
    }
    wpt->altitude.first = r["altitude"].to(wpt->altitude.second);
    wpt->symbol.first = r["symbol"].to(wpt->symbol.second);
    wpt->comment.first = r["comment"].to(wpt->comment.second);
    wpt->description.first = r["description"].to(wpt->description.second);
    wpt->color.first = r["color"].to(wpt->color.second);
    wpt->type.first = r["type"].to(wpt->type.second);
    wpt->avg_samples.first = r["avg_samples"].to(wpt->avg_samples.second);
    waypoints.push_back(std::move(wpt));
  }
  tx.commit();
  return waypoints;
}

void ItineraryPgDao::create_waypoints(
    work &tx,
    long itinerary_id,
    std::vector<std::shared_ptr<waypoint>> waypoints)
{
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
    if (w->time.first)
      timestr = DateTime(w->time.second).get_time_as_iso8601_gmt();
    tx.exec_prepared(
        ps_name,
        itinerary_id,
        w->name.first ? &w->name.second : nullptr,
        w->longitude,
        w->latitude,
        w->altitude.first ? &w->altitude.second : nullptr,
        w->time.first ? &timestr : nullptr,
        w->comment.first ? &w->comment.second : nullptr,
        w->description.first ? &w->description.second : nullptr,
        w->symbol.first ? &w->symbol.second : nullptr,
        w->color.first ? &w->color.second : nullptr,
        w->type.first ? &w->type.second : nullptr,
        w->avg_samples.first ? &w->avg_samples.second : nullptr);
  }
}

void ItineraryPgDao::create_route_points(
    work &tx,
    std::shared_ptr<route> route)
{
  const std::string ps_name = "create-route-points";
  const std::string sql =
    "INSERT INTO itinerary_route_point "
    "(itinerary_route_id, geog, altitude, name, "
    "comment, description, symbol) "
    "VALUES ($1, ST_SetSRID(ST_POINT($2, $3),4326), $4, $5, $6, $7, $8) "
    "RETURNING id";
  for (const auto &p : route->points) {
    auto r = tx.exec_params1(
        sql,
        route->id.first ? &route->id.second : nullptr,
        p->longitude,
        p->latitude,
        p->altitude.first ? &p->altitude.second : nullptr,
        p->name.first ? &p->name.second : nullptr,
        p->comment.first ? &p->comment.second : nullptr,
        p->description.first ? &p->description.second : nullptr,
        p->symbol.first ? &p->symbol.second : nullptr);
    p->id.first = r["id"].to(p->id.second);
  }
}

void ItineraryPgDao::create_routes(
    work &tx,
    long itinerary_id,
    std::vector<std::shared_ptr<route>> routes)
{
  const std::string ps_name = "create-routes";
  const std::string sql =
    "INSERT INTO itinerary_route "
    "(itinerary_id, name, color, distance, ascent, descent, lowest, highest) "
    "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id";
  for (const auto &r : routes) {
    auto result = tx.exec_params1(
        sql,
        itinerary_id,
        r->name.first ? &r->name.second : nullptr,
        r->color.first ? &r->color.second : nullptr,
        r->distance.first ? &r->distance.second : nullptr,
        r->ascent.first ? &r->ascent.second : nullptr,
        r->descent.first ? &r->descent.second : nullptr,
        r->lowest.first ? &r->lowest.second : nullptr,
        r->highest.first ? &r->highest.second : nullptr);
    r->id.first = result["id"].to(r->id.second);
    create_route_points(tx, r);
  }
}

void ItineraryPgDao::create_track_points(
    work &tx,
    std::shared_ptr<track_segment> segment)
{
  const std::string ps_name = "create-track-points";
  const std::string sql =
    "INSERT INTO itinerary_track_point "
    "(itinerary_track_segment_id, geog, time, hdop, altitude) "
    "VALUES ($1, ST_SetSRID(ST_POINT($2, $3),4326), $4, $5, $6) "
    "RETURNING id";
  for (const auto &p : segment->points) {
    std::string timestr;
    if (p->time.first)
      timestr = DateTime(p->time.second).get_time_as_iso8601_gmt();
    auto r = tx.exec_params1(
        sql,
        segment->id.first ? &segment->id.second : nullptr,
        p->longitude,
        p->latitude,
        p->time.first ? &timestr : nullptr,
        p->hdop.first ? &p->hdop.second : nullptr,
        p->altitude.first ? &p->altitude.second : nullptr);
    p->id.first = r["id"].to(p->id.second);
  }
}

void ItineraryPgDao::create_track_segments(
    work &tx,
    std::shared_ptr<track> track)
{
  const std::string ps_name = "create-track-segments";
  const std::string sql =
    "INSERT INTO itinerary_track_segment "
    "(itinerary_track_id, distance, ascent, descent, lowest, highest) "
    "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id";
  for (const auto &seg : track->segments) {
    auto r = tx.exec_params1(
        sql,
        track->id.first ? &track->id.second : nullptr,
        seg->distance.first ? &seg->distance.second : nullptr,
        seg->ascent.first ? &seg->ascent.second : nullptr,
        seg->descent.first ? &seg->descent.second : nullptr,
        seg->lowest.first ? &seg->lowest.second : nullptr,
        seg->highest.first ? &seg->highest.second : nullptr);
    seg->id.first = r["id"].to(seg->id.second);
    create_track_points(tx, seg);
  }
}

void ItineraryPgDao::create_tracks(
    work &tx,
    long itinerary_id,
    std::vector<std::shared_ptr<track>> tracks)
{
  const std::string ps_name = "create-tracks";
  const std::string sql =
    "INSERT INTO itinerary_track "
    "(itinerary_id, name, color, distance, ascent, descent, lowest, highest) "
    "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id";
  for (const auto &t : tracks) {
    auto r = tx.exec_params1(
        sql,
        itinerary_id,
        t->name.first ? &t->name.second : nullptr,
        t->color.first ? &t->color.second : nullptr,
        t->distance.first ? &t->distance.second : nullptr,
        t->ascent.first ? &t->ascent.second : nullptr,
        t->descent.first ? &t->descent.second : nullptr,
        t->lowest.first ? &t->lowest.second : nullptr,
        t->highest.first ? &t->highest.second : nullptr);
    t->id.first = r["id"].to(t->id.second);
    create_track_segments(tx, t);
  }
}

void ItineraryPgDao::create_itinerary_features(
    std::string user_id,
    long itinerary_id,
    const ItineraryPgDao::itinerary_features &features)
{
  work tx(*connection);
  try {
    auto r = tx.exec_params(
        "SELECT COUNT(*) FROM itinerary "
        "WHERE user_id=$1 AND id=$2", user_id, itinerary_id);
    if (r[0][0].as<long>() < 1)
      throw NotAuthorized();
    create_waypoints(tx, itinerary_id, features.waypoints);
    create_routes(tx, itinerary_id, features.routes);
    create_tracks(tx, itinerary_id, features.tracks);
    tx.commit();
  } catch (const std::exception &e) {
    std::cerr << "Exception whilst saving itinerary featues: "
              << e.what();
    throw;
  }
}
