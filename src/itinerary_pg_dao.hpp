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
#ifndef ITINERARY_PG_DAO_HPP
#define ITINERARY_PG_DAO_HPP

#include "trip_pg_dao.hpp"
#include "geo_utils.hpp"
#include <chrono>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace fdsd {
namespace trip {

class ItineraryPgDao : public TripPgDao {
public:
  struct path_base : public path_statistics {
    std::pair<bool, long> id;
    path_base() :
      path_statistics(),
      id() {}
  };
  struct path_summary : public path_base {
    std::pair<bool, std::string> name;
    std::pair<bool, std::string> color;
    std::pair<bool, std::string> html_code;
    path_summary() :
      path_base(),
      name(),
      color(),
      html_code() {}
    static void from_geojson_properties(
        const nlohmann::json& properties,
        ItineraryPgDao::path_summary &path);
  };
  struct path_info {
    std::pair<bool, double> minimum_speed;
    std::pair<bool, double> maximum_speed;
    std::pair<bool, double> average_speed;
    path_info() :
      minimum_speed(),
      maximum_speed(),
      average_speed() {}
  };
  struct point_info {
    std::pair<bool, double> bearing;
    std::pair<bool, double> speed;
    point_info() : bearing(), speed() {}
  };
  struct route_point : public location {
    std::pair<bool, std::string> name;
    std::pair<bool, std::string> comment;
    std::pair<bool, std::string> description;
    std::pair<bool, std::string> symbol;
    route_point() :
      location(),
      name(),
      comment(),
      description(),
      symbol() {}
    route_point(location loc) :
      location(loc),
      name(),
      comment(),
      description(),
      symbol() {}
  };
  struct track;
  struct route : public path_summary {
    std::vector<route_point> points;
    route() : path_summary(), points() {}
    route(const track &t);
    void calculate_statistics();
    static void calculate_statistics(std::vector<route> &routes) {
      for (auto &route : routes)
        route.calculate_statistics();
    }
  };
  struct track_point : public location {
    std::pair<bool, std::chrono::system_clock::time_point> time;
    std::pair<bool, float> hdop;
    track_point() : location(), time(), hdop() {}
    track_point(location loc) : location(loc), time(), hdop() {}
  };
  struct track_segment : public path_base {
    std::vector<track_point> points;
    track_segment() : path_base(), points() {}
    track_segment(std::vector<track_point> points)
      : path_base(), points(std::move(points)) {}
  };
  struct track : public path_summary {
    std::vector<track_segment> segments;
    track() : path_summary(), segments() {}
    track(std::vector<track_segment> segments)
      : path_summary(), segments(std::move(segments)) {}
    track(const route &r);
    void calculate_statistics();
    static void calculate_statistics(std::vector<track> &tracks) {
      for (auto &track : tracks)
        track.calculate_statistics();
    }
  };
  struct waypoint_base {
    std::pair<bool, std::string> name;
    std::pair<bool, std::string> comment;
    std::pair<bool, std::string> symbol;
    std::pair<bool, std::string> type;
    waypoint_base() :
      name(),
      comment(),
      symbol(),
      type() {}
  };
  struct waypoint_summary : public waypoint_base {
    long id;
    waypoint_summary() : waypoint_base(), id() {}
  };
  struct waypoint : public location, public waypoint_base {
    std::pair<bool, std::chrono::system_clock::time_point> time;
    std::pair<bool, std::string> description;
    std::pair<bool, std::string> color;
    std::pair<bool, std::string> type;
    std::pair<bool, long> avg_samples;
    waypoint() : location(),
                 waypoint_base(),
                 time(),
                 description(),
                 color(),
                 type(),
                 avg_samples() {}
  };
  struct itinerary_features {
    std::vector<route> routes;
    std::vector<track> tracks;
    std::vector<waypoint> waypoints;
    itinerary_features() : routes(),
                           tracks(),
                           waypoints() {}
  };
  struct itinerary_base {
    std::pair<bool, long> id;
    std::pair<bool, std::chrono::system_clock::time_point> start;
    std::pair<bool, std::chrono::system_clock::time_point> finish;
    std::string title;
    itinerary_base() : id(),
                       start(),
                       finish() {}
  };
  struct itinerary_summary : public itinerary_base {
    std::pair<bool, std::string> owner_nickname;
    std::pair<bool, bool> shared;
    itinerary_summary() : itinerary_base(),
                          owner_nickname(),
                          shared() {}
  };
  struct itinerary_description : public itinerary_summary {
    std::pair<bool, std::string> description;
    itinerary_description() : itinerary_summary(),
                              description() {}
  };
  struct itinerary_detail : public itinerary_description {
    std::pair<bool, std::string> shared_to_nickname;
  };
  struct itinerary : public itinerary_detail {
    std::pair<bool, std::string> description;
    std::vector<path_summary> routes;
    std::vector<path_summary> tracks;
    std::vector<waypoint_summary> waypoints;
    itinerary() : itinerary_detail(),
                  description(),
                  routes(),
                  tracks(),
                  waypoints() {}
  };
  struct selected_feature_ids {
    std::vector<long> routes;
    std::vector<long> tracks;
    std::vector<long> waypoints;
    selected_feature_ids() : routes(), tracks(), waypoints() {}
    static void to_json(nlohmann::json& j, const selected_feature_ids& ids);
    static void from_json(const nlohmann::json& j, selected_feature_ids& ids);
  };
  long get_itineraries_count(
      std::string user_id);
  std::vector<itinerary_summary> get_itineraries(
      std::string user_id,
      std::uint32_t offset,
      int limit);
  std::pair<bool, ItineraryPgDao::itinerary>
      get_itinerary_details(
          std::string user_id, long itinerary_id);
  std::pair<bool, ItineraryPgDao::itinerary_description>
      get_itinerary_description(
          std::string user_id, long itinerary_id);
  long save_itinerary(
      std::string user_id,
      ItineraryPgDao::itinerary_description itinerary);
  void delete_itinerary(
      std::string user_id,
      long itinerary_id);
  void create_itinerary_features(
      std::string user_id,
      long itinerary_id,
      ItineraryPgDao::itinerary_features &features);
  std::vector<route>
      get_routes(std::string user_id,
                 long itinerary_id,
                 std::vector<long> route_ids);
  std::vector<track>
      get_tracks(std::string user_id,
                 long itinerary_id,
                 std::vector<long> ids);
  std::vector<ItineraryPgDao::waypoint>
      get_waypoints(std::string user_id,
                    long itinerary_id,
                    std::vector<long> ids);
  waypoint get_waypoint(
      std::string user_id,
      long itinerary_id,
      long waypoint_id);

  void save(std::string user_id,
            long itinerary_id,
            waypoint &wpt);

  void save(std::string user_id,
            long itinerary_id,
            route &route);

  bool has_user_itinerary_modification_access(std::string user_id,
                                              long itinerary_id);
  void validate_user_itinerary_modification_access(std::string user_id,
                                                   long itinerary_id);
  void validate_user_itinerary_read_access(std::string user_id,
                                           long itinerary_id);
  void delete_features(
      std::string user_id,
      long itinerary_id,
      const selected_feature_ids &features);

  std::vector<std::pair<std::string, std::string>>
      get_georef_formats();

  std::vector<std::pair<std::string, std::string>>
      get_waypoint_symbols();

  void auto_color_paths(std::string user_id,
                        long itinerary_id,
                        const selected_feature_ids &selected);
protected:
  void create_waypoints(
      pqxx::work &tx,
      long itinerary_id,
      const std::vector<ItineraryPgDao::waypoint> &waypoints);
  void create_route_points(
      pqxx::work &tx,
      route &route);
  void create_routes(
      pqxx::work &tx,
      long itinerary_id,
      std::vector<route> &routes);
  void create_track_points(
      pqxx::work &tx,
      track_segment &segment);
  void create_track_segments(
      pqxx::work &tx,
      track &track);
  void create_tracks(
      pqxx::work &tx,
      long itinerary_id,
      std::vector<track> &tracks);
  void validate_user_itinerary_modification_access(pqxx::work &tx,
                                                   std::string user_id,
                                                   long itinerary_id);
  void validate_user_itinerary_read_access(pqxx::work &tx,
                                           std::string user_id,
                                           long itinerary_id);
};

/// Allows Argument-depenedent lookup for the nlohmann/json library to find this method
void to_json(nlohmann::json& j, const ItineraryPgDao::selected_feature_ids& ids);
/// Allows Argument-depenedent lookup for the nlohmann/json library to find this method
void from_json(const nlohmann::json& j,  ItineraryPgDao::selected_feature_ids& ids);

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_PG_DAO_HPP
