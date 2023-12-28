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

#include "geo_utils.hpp"
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

struct time_span_type {
  /// False until a time span has been set
  bool is_valid;
  std::chrono::system_clock::time_point start;
  std::chrono::system_clock::time_point finish;

  time_span_type() : is_valid(false), start(), finish() {}
  time_span_type(const std::chrono::system_clock::time_point start,
                 const std::chrono::system_clock::time_point finish)
    : is_valid(true), start(start), finish(finish) {}

  void update_start(const std::chrono::system_clock::time_point start);

  void update_finish(const std::chrono::system_clock::time_point finish);

  void update(const std::chrono::system_clock::time_point time) {
    update_start(time);
    update_finish(time);
  }

  /// Updates this time span to encompass the range of the passed time span
  void update_time_span(const time_span_type &other);

};

class ItineraryPgDao : public TripPgDao {
  static const std::string itinerary_waypoint_radius_clause;
  static const std::string itinerary_route_radius_clause;
  static const std::string itinerary_track_radius_clause;
  static const std::string shared_itinerary_waypoint_radius_clause;
  static const std::string shared_itinerary_route_radius_clause;
  static const std::string shared_itinerary_track_radius_clause;
  static const std::string itinerary_search_query_body;
  static const std::string itinerary_share_report_query_body;
public:
  struct path_base : public path_statistics {
    std::pair<bool, long> id;
    path_base() :
      path_statistics(),
      id() {}
    static YAML::Node encode(const path_base& rhs);
    static bool decode(const YAML::Node& node, path_base& rhs);
  };
  struct path_color {
    std::string key;
    std::string description;
    std::string html_code;
    static void to_json(nlohmann::json& j, const path_color& p);
    static void from_json(const nlohmann::json& j, path_color& p);
    path_color() :
      key(),
      description(),
      html_code() {}
  };
  struct path_summary : public path_base {
    std::pair<bool, std::string> name;
    /// The color key for looking up the color details, e.g. 'DarkBlue'
    std::pair<bool, std::string> color_key;
    /// The descriptive name of the color, e.g. 'Dark Blue'
    std::pair<bool, std::string> color_description;
    /// The HTML code to render the color in HTML. e.g. 'navy'
    std::pair<bool, std::string> html_code;
    path_summary() :
      path_base(),
      name(),
      color_key(),
      color_description(),
      html_code() {}
    static void from_geojson_properties(
        const nlohmann::json& properties,
        ItineraryPgDao::path_summary &path);
    static YAML::Node encode(const path_summary& rhs);
    static bool decode(const YAML::Node& node, path_summary& rhs);
  };
  struct point_info : location {
    std::pair<bool, double> bearing;
    point_info() : location(), bearing() {}
    point_info(location loc) : location(loc), bearing() {}
    void calculate_bearing(const point_info &previous);
  };
  struct route_point : public point_info {
    std::pair<bool, std::string> name;
    std::pair<bool, std::string> comment;
    std::pair<bool, std::string> description;
    std::pair<bool, std::string> symbol;
    route_point() :
      point_info(),
      name(),
      comment(),
      description(),
      symbol() {}
    route_point(location loc) :
      point_info(loc),
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
  struct track_point : public point_info {
    std::pair<bool, std::chrono::system_clock::time_point> time;
    std::pair<bool, float> hdop;
    std::pair<bool, double> speed;
    track_point() : point_info(), time(), hdop(), speed() {}
    track_point(location loc) : point_info(loc), time(), hdop(), speed() {}
    track_point(const TrackPgDao::tracked_location &loc);
    void calculate_speed(const track_point &previous);
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
    void calculate_speed_and_bearing_values();
    std::pair<bool, double> calculate_maximum_speed() const;
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
    std::pair<bool, std::string> extended_attributes;
    std::pair<bool, std::string> type;
    std::pair<bool, long> avg_samples;
    waypoint() : location(),
                 waypoint_base(),
                 time(),
                 description(),
                 extended_attributes(),
                 type(),
                 avg_samples() {}
    waypoint(location loc)
      : location(loc),
        waypoint_base(),
        time(),
        description(),
        extended_attributes(),
        type(),
        avg_samples() {}
    waypoint(TrackPgDao::tracked_location loc)
      : location(loc),
        waypoint_base(),
        time(std::make_pair(true, loc.time_point)),
        description(),
        extended_attributes(),
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
  struct itinerary_share_report : public itinerary_base {
    std::vector<std::string> nicknames;
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
  struct itinerary_complete : public itinerary_detail {
    std::vector<route> routes;
    std::vector<waypoint> waypoints;
    std::vector<track> tracks;
    std::vector<itinerary_share> shares;
    itinerary_complete()
      : itinerary_detail(),
        routes(),
        waypoints(),
        tracks(),
        shares() {}
    itinerary_complete(itinerary_detail itinerary)
      : itinerary_detail(itinerary),
        routes(),
        waypoints(),
        tracks(),
        shares() {}
    static YAML::Node encode(const itinerary_complete& rhs);
    static bool decode(const YAML::Node& node, itinerary_complete& rhs);
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
  /// Fetches basic detials of an itinerary excluding related child data
  std::pair<bool, itinerary_description>
      get_itinerary_description(
          std::string user_id, long itinerary_id);
  /// Fetches itinerary and itinerary shares including basic summary of routes,
  /// waypoints and tracks
  std::pair<bool, ItineraryPgDao::itinerary>
      get_itinerary_summary(
          std::string user_id, long itinerary_id);
  std::pair<bool, ItineraryPgDao::itinerary_complete>
      get_itinerary_complete(
          std::string user_id, long itinerary_id);
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
                 const std::vector<long> &route_ids);

  route get_route(std::string user_id,
                  long itinerary_id,
                  long route_id) {
    std::vector<long> route_ids;
    route_ids.push_back(route_id);
    auto routes = get_routes(user_id, itinerary_id, route_ids);
    return routes.front();
  }
  std::vector<route>
      get_routes(std::string user_id,
                 long itinerary_id);
  path_summary get_route_summary(std::string user_id,
                                 long itinerary_id,
                                 long route_id);
  std::vector<ItineraryPgDao::path_summary> get_route_summaries(
      std::string user_id,
      long itinerary_id,
      std::vector<long> route_id);
  std::vector<track>
      get_tracks(std::string user_id,
                 long itinerary_id,
                 const std::vector<long> &ids);
  track get_track(std::string user_id,
                  long itinerary_id,
                  long track_id) {
    std::vector<long> track_ids;
    track_ids.push_back(track_id);
    auto tracks = get_tracks(user_id, itinerary_id, track_ids);
    return tracks.front();
  }
  long get_track_segment_count(std::string user_id,
                               long itinerary_id,
                               long track_id);
  /// \return a track with a summary of the track and a list of summarized
  /// track segments
  track get_track_segments(std::string user_id,
                           long itinerary_id,
                           long track_id,
                           std::uint32_t offset,
                           int limit);
  /// \return a count of track segment points for the specified track segment
  long get_track_segment_point_count(
      std::string user_id,
      long itinerary_id,
      long track_segment_id);
  /// \return a track segment with a summary of the segment and a list of
  /// summarized track segment points
  track_segment get_track_segment(
      std::string user_id,
      long itinerary_id,
      long track_segment_id,
      std::uint32_t offset,
      int limit);
  /// \return list of track_segment objects for the passed list of segment IDs
  std::vector<track_segment> get_track_segments(
      std::string user_id,
      long itinerary_id,
      long track_id,
      std::vector<long> track_segment_ids);
  /// \return list of track_points for the passed list of itinerary_track_point IDs
  std::vector<ItineraryPgDao::track_point>
      get_track_points(
          std::string user_id,
          long itinerary_id,
          const std::vector<long> &point_ids);
  long get_route_point_count(
      std::string user_id,
      long itinerary_id,
      long route_id);
  route get_route_points(
      std::string user_id,
      long itinerary_id,
      long route_id,
      std::uint32_t offset,
      int limit);
  /// \return list of route_points for the passed list of itinerary_route_point IDs
  std::vector<ItineraryPgDao::route_point>
      get_route_points(
          std::string user_id,
          long itinerary_id,
          const std::vector<long> &point_ids);
  std::vector<track>
      get_tracks(std::string user_id,
                 long itinerary_id);
  path_summary get_track_summary(std::string user_id,
                          long itinerary_id,
                          long track_id);
  std::vector<ItineraryPgDao::path_summary> get_track_summaries(
      std::string user_id,
      long itinerary_id,
      std::vector<long> track_ids);
  std::vector<ItineraryPgDao::waypoint>
      get_waypoints(std::string user_id,
                    long itinerary_id,
                    const std::vector<long> &ids);
  std::vector<ItineraryPgDao::waypoint>
      get_waypoints(std::string user_id,
                    long itinerary_id);
  waypoint get_waypoint(
      std::string user_id,
      long itinerary_id,
      long waypoint_id);

  long save(
      std::string user_id,
      itinerary_description itinerary);

  void save(std::string user_id,
            long itinerary_id,
            waypoint &wpt);

  void save(std::string user_id,
            long itinerary_id,
            route &route);

  void save(std::string user_id,
            long itinerary_id,
            track &track);

  void update_route_summary(std::string user_id,
                            long itinerary_id,
                            const path_summary &route);

  void update_track_summary(std::string user_id,
                            long itinerary_id,
                            const path_summary &track);

  void save_updated_statistics(
      std::string user_id,
      long itinerary_id,
      const track &track);

  void save(std::string user_id,
            long itinerary_id,
            itinerary_share &share);

  void save(std::string user_id,
            long itinerary_id,
            std::vector<itinerary_share> &shares);

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

  std::vector<path_color>
      get_path_colors();

  std::vector<std::pair<std::string, std::string>>
      get_path_color_options();

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
  long itinerary_radius_search_count(
      std::string user_id,
      double longitude,
      double latitude,
      double radius);

  std::vector<ItineraryPgDao::itinerary_summary>
      itinerary_radius_search(
          std::string user_id,
          double longitude,
          double latitude,
          double radius,
          std::uint32_t offset,
          int limit);

  long get_shared_itinerary_report_count(std::string user_id);

  std::vector<ItineraryPgDao::itinerary_share_report>
      get_shared_itinerary_report(
          std::string user_id,
          std::uint32_t offset,
          int limit);

  std::vector<std::string>
      get_nicknames_sharing_location_with_user(std::string user_id);

  static void update_time_span(
      time_span_type &time_span,
      const std::vector<ItineraryPgDao::track> &tracks);

  static void update_time_span(
      time_span_type &time_span,
      const std::vector<ItineraryPgDao::waypoint> &waypoints);

  static time_span_type get_time_span(
      const std::vector<ItineraryPgDao::track> &tracks,
      const std::vector<ItineraryPgDao::waypoint> &waypoints);

  static std::pair<bool, bounding_box> get_bounding_box(
      const std::vector<ItineraryPgDao::track> &tracks,
      const std::vector<ItineraryPgDao::route> &routes,
      const std::vector<ItineraryPgDao::waypoint> &waypoints);

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
  bool has_user_itinerary_modification_access(pqxx::work &tx,
                                              std::string user_id,
                                              long itinerary_id);
  void validate_user_itinerary_modification_access(pqxx::work &tx,
                                                   std::string user_id,
                                                   long itinerary_id);
  void validate_user_itinerary_read_access(pqxx::work &tx,
                                           std::string user_id,
                                           long itinerary_id);
  /// Fetches itinerary and itinerary shares
  std::pair<bool, ItineraryPgDao::itinerary_detail>
      get_itinerary_details(pqxx::work &tx,
                            std::string user_id,
                            long itinerary_id);
  std::vector<itinerary_share> get_itinerary_shares(pqxx::work &tx,
                                                    std::string user_id,
                                                    long itinerary_id,
                                                    std::uint32_t offset = 0,
                                                    int limit = -1);
  std::vector<route>
      get_routes(pqxx::work &tx,
                 std::string user_id,
                 long itinerary_id);
  std::vector<track>
      get_tracks(pqxx::work &tx,
                 std::string user_id,
                 long itinerary_id);
  std::vector<ItineraryPgDao::waypoint>
      get_waypoints(pqxx::work &tx,
                    std::string user_id,
                    long itinerary_id);
public:
  void create_waypoints(
      std::string user_id,
      long itinerary_id,
      const std::vector<ItineraryPgDao::waypoint> &waypoints);
  void create_routes(
      std::string user_id,
      long itinerary_id,
      std::vector<route> &routes);

  void create_route(
      std::string user_id, long itinerary_id, const route &route) {
    std::vector<ItineraryPgDao::route> routes;
    routes.push_back(route);
    create_routes(user_id, itinerary_id, routes);
  }

  void create_tracks(
      std::string user_id,
      long itinerary_id,
      std::vector<track> &tracks);

  void create_track(std::string user_id, long itinerary_id, track &track) {
    std::vector<ItineraryPgDao::track> tracks;
    tracks.push_back(track);
    create_tracks(user_id, itinerary_id, tracks);
  }

};

/// Allows Argument-depenedent lookup for the nlohmann/json library to find these methods
void to_json(nlohmann::json& j, const ItineraryPgDao::selected_feature_ids& ids);
void from_json(const nlohmann::json& j,  ItineraryPgDao::selected_feature_ids& ids);
void to_json(nlohmann::json& j, const ItineraryPgDao::path_color& p);
void from_json(const nlohmann::json& j,  ItineraryPgDao::path_color& p);

} // namespace trip
} // namespace fdsd

namespace YAML {

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::itinerary_description> {

    static Node encode(
        const fdsd::trip::ItineraryPgDao::itinerary_description& rhs) {
      return fdsd::trip::ItineraryPgDao::itinerary_description::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::itinerary_description& rhs) {
      return
        fdsd::trip::ItineraryPgDao::itinerary_description::decode(node, rhs);
    }
  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::itinerary_complete> {

    static Node encode(
        const fdsd::trip::ItineraryPgDao::itinerary_complete& rhs) {
      return fdsd::trip::ItineraryPgDao::itinerary_complete::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::itinerary_complete& rhs) {
      return fdsd::trip::ItineraryPgDao::itinerary_complete::decode(node, rhs);
    }
  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::route_point> {

    static Node encode(const fdsd::trip::ItineraryPgDao::route_point& rhs) {
      return fdsd::trip::ItineraryPgDao::route_point::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::route_point& rhs) {
      return fdsd::trip::ItineraryPgDao::route_point::decode(node, rhs);
    }
  };

  template<>
  struct convert<fdsd::trip::ItineraryPgDao::itinerary_share> {

    static Node encode(const fdsd::trip::ItineraryPgDao::itinerary_share& rhs) {
      return fdsd::trip::ItineraryPgDao::itinerary_share::encode(rhs);
    }

    static bool decode(const Node& node,
                       fdsd::trip::ItineraryPgDao::itinerary_share& rhs) {
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
