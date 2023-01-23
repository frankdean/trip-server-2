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
#include "tracking_pg_dao.hpp"
#include "geo_utils.hpp"
#include <chrono>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

namespace fdsd {
namespace trip {

class ItineraryPgDao : public TripPgDao {
public:
  struct path_base : public path_statistics {
    std::pair<bool, long> id;
    path_base() :
      path_statistics(),
      id() {}
    static YAML::Node encode(const path_base& rhs);
    static bool decode(const YAML::Node& node, path_base& rhs);
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
    static YAML::Node encode(const path_summary& rhs);
    static bool decode(const YAML::Node& node, path_summary& rhs);
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
    static YAML::Node encode(const route_point& rhs);
    static bool decode(const YAML::Node& node, route_point& rhs);
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
    static YAML::Node encode(const route& rhs);
    static bool decode(const YAML::Node& node, route& rhs);
  };
  struct track_point : public location {
    std::pair<bool, std::chrono::system_clock::time_point> time;
    std::pair<bool, float> hdop;
    track_point() : location(), time(), hdop() {}
    track_point(location loc) : location(loc), time(), hdop() {}
    static YAML::Node encode(const track_point& rhs);
    static bool decode(const YAML::Node& node, track_point& rhs);
  };
  struct track_segment : public path_base {
    std::vector<track_point> points;
    track_segment() : path_base(), points() {}
    track_segment(std::vector<track_point> points)
      : path_base(), points(std::move(points)) {}
    static YAML::Node encode(const track_segment& rhs);
    static bool decode(const YAML::Node& node, track_segment& rhs);
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
    static YAML::Node encode(const track& rhs);
    static bool decode(const YAML::Node& node, track& rhs);
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
    static YAML::Node encode(const waypoint_base& rhs);
    static bool decode(const YAML::Node& node, waypoint_base& rhs);
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
    waypoint(location loc)
      : location(loc),
        waypoint_base(),
        time(),
        description(),
        color(),
        type(),
        avg_samples() {}
    waypoint(TrackPgDao::tracked_location loc)
      : location(loc),
        waypoint_base(),
        time(std::make_pair(true, loc.time_point)),
        description(),
        color(),
        type(),
        avg_samples() {
      comment = loc.note;
    }
    static YAML::Node encode(const waypoint& rhs);
    static bool decode(const YAML::Node& node, waypoint& rhs);
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
    static YAML::Node encode(const itinerary_base& rhs);
    static bool decode(const YAML::Node& node, itinerary_base& rhs);
  };
  struct itinerary_summary : public itinerary_base {
    std::pair<bool, std::string> owner_nickname;
    /// true when shared to the current user by another user
    std::pair<bool, bool> shared;
    itinerary_summary() : itinerary_base(),
                          owner_nickname(),
                          shared() {}
    static YAML::Node encode(const itinerary_summary& rhs);
    static bool decode(const YAML::Node& node, itinerary_summary& rhs);
  };
  struct itinerary_description : public itinerary_summary {
    std::pair<bool, std::string> description;
    itinerary_description() : itinerary_summary(),
                              description() {}
    static YAML::Node encode(const itinerary_description& rhs);
    static bool decode(const YAML::Node& node, itinerary_description& rhs);
  };
  struct itinerary_detail : public itinerary_description {
    /// Will be set and contain the current user when the itinerary belongs to
    /// another user
    std::pair<bool, std::string> shared_to_nickname;
    static YAML::Node encode(const itinerary_detail& rhs);
    static bool decode(const YAML::Node& node, itinerary_detail& rhs);
  };
  struct itinerary : public itinerary_detail {
    std::vector<path_summary> routes;
    std::vector<path_summary> tracks;
    std::vector<waypoint_summary> waypoints;
    itinerary() : itinerary_detail(),
                  routes(),
                  tracks(),
                  waypoints() {}
    itinerary(itinerary_detail detail) : itinerary_detail(detail),
                                         routes(),
                                         tracks(),
                                         waypoints() {}
  };
  struct itinerary_share {
    long shared_to_id;
    std::string nickname;
    std::pair<bool, bool> active;
    itinerary_share() : shared_to_id(),
                        nickname(),
                        active() {}
    static YAML::Node encode(const itinerary_share& rhs);
    static bool decode(const YAML::Node& node, itinerary_share& rhs);
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
  std::pair<bool, ItineraryPgDao::itinerary_detail>
      get_itinerary_details_and_summary(
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
  std::vector<route>
      get_routes(std::string user_id,
                 long itinerary_id);
  std::vector<track>
      get_tracks(std::string user_id,
                 long itinerary_id,
                 std::vector<long> ids);
  std::vector<track>
      get_tracks(std::string user_id,
                 long itinerary_id);
  std::vector<ItineraryPgDao::waypoint>
      get_waypoints(std::string user_id,
                    long itinerary_id,
                    std::vector<long> ids);
  std::vector<ItineraryPgDao::waypoint>
      get_waypoints(std::string user_id,
                    long itinerary_id);
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

  void save(std::string user_id,
            long itinerary_id,
            itinerary_share &share);

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

  long get_itinerary_shares_count(std::string user_id,
                                 long itinerary_id);

  std::vector<itinerary_share> get_itinerary_shares(std::string user_id,
                                                    long itinerary_id,
                                                    std::uint32_t offset = 0,
                                                    int limit = -1);

  itinerary_share get_itinerary_share(std::string user_id,
                                      long itinerary_id,
                                      long shared_to_id);

  void activate_itinerary_shares(std::string user_id,
                                 long itinerary_id,
                                 const std::vector<long> &shared_to_ids,
                                 bool activate);
  void delete_itinerary_shares(std::string user_id,
                               long itinerary_id,
                               const std::vector<long> &shared_to_ids);

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
  std::pair<bool, ItineraryPgDao::itinerary_detail>
      get_itinerary_details_and_summary(pqxx::work &tx,
                                        std::string user_id,
                                        long itinerary_id);
public:
  void create_waypoints(
      std::string user_id,
      long itinerary_id,
      const std::vector<ItineraryPgDao::waypoint> &waypoints);
  void create_tracks(
      std::string user_id,
      long itinerary_id,
      std::vector<track> &tracks);
};

/// Allows Argument-depenedent lookup for the nlohmann/json library to find this method
void to_json(nlohmann::json& j, const ItineraryPgDao::selected_feature_ids& ids);
/// Allows Argument-depenedent lookup for the nlohmann/json library to find this method
void from_json(const nlohmann::json& j,  ItineraryPgDao::selected_feature_ids& ids);

} // namespace trip
} // namespace fdsd

namespace YAML {

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::route_point> {

    static Node encode(const fdsd::trip::ItineraryPgDao::route_point& rhs) {
      return fdsd::trip::ItineraryPgDao::route_point::encode(rhs);
    }

    static bool decode(const Node& node, fdsd::trip::ItineraryPgDao::route_point& rhs) {
      return fdsd::trip::ItineraryPgDao::route_point::decode(node, rhs);
    }
  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::itinerary_share> {

    static Node encode(const fdsd::trip::ItineraryPgDao::itinerary_share& rhs) {
      return fdsd::trip::ItineraryPgDao::itinerary_share::encode(rhs);
    }

    static bool decode(const Node& node, fdsd::trip::ItineraryPgDao::itinerary_share& rhs) {
      return fdsd::trip::ItineraryPgDao::itinerary_share::decode(node, rhs);
    }

  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::path_summary> {

    static Node encode(const fdsd::trip::ItineraryPgDao::path_summary& rhs) {
      return fdsd::trip::ItineraryPgDao::path_summary::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::path_summary& rhs) {
      return fdsd::trip::ItineraryPgDao::path_summary::decode(node, rhs);
    }
  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::route> {

    static Node encode(const fdsd::trip::ItineraryPgDao::route& rhs) {
      return fdsd::trip::ItineraryPgDao::route::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::route& rhs) {
      return fdsd::trip::ItineraryPgDao::route::decode(node, rhs);
    }
  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::waypoint> {

    static Node encode(const fdsd::trip::ItineraryPgDao::waypoint& rhs) {
      return fdsd::trip::ItineraryPgDao::waypoint::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::waypoint& rhs) {
      return fdsd::trip::ItineraryPgDao::waypoint::decode(node, rhs);
    }
  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::track> {

    static Node encode(const fdsd::trip::ItineraryPgDao::track& rhs) {
      return fdsd::trip::ItineraryPgDao::track::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::track& rhs) {
      return fdsd::trip::ItineraryPgDao::track::decode(node, rhs);
    }
  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::track_segment> {

    static Node encode(const fdsd::trip::ItineraryPgDao::track_segment& rhs) {
      return fdsd::trip::ItineraryPgDao::track_segment::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::track_segment& rhs) {
      return fdsd::trip::ItineraryPgDao::track_segment::decode(node, rhs);
    }
  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::track_point> {

    static Node encode(const fdsd::trip::ItineraryPgDao::track_point& rhs) {
      return fdsd::trip::ItineraryPgDao::track_point::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::track_point& rhs) {
      return fdsd::trip::ItineraryPgDao::track_point::decode(node, rhs);
    }
  };

} // namespace YAML

#endif // ITINERARY_PG_DAO_HPP
