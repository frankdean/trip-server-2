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
#ifndef TRIP_CONFIG_HPP
#define TRIP_CONFIG_HPP

#include "../trip-server-common/src/http_client.hpp"
#include <string>
#include <yaml-cpp/yaml.h>

namespace fdsd {
namespace trip {

struct tile_provider : public fdsd::web::HttpOptions {
  virtual ~tile_provider() {}
  bool cache = true;
  bool prune = true;
  std::string help;
  std::string user_agent_info;
  std::string referrer_info;
  std::string name;
  std::string type;
  int min_zoom;
  int max_zoom;
  std::string tile_attributions_html;
  virtual std::string to_string() const override;
  inline friend std::ostream& operator<<
      (std::ostream& out, const tile_provider& rhs) {
    return out << rhs.to_string();
  }
};

class TripConfig {
  std::string root_directory;
  std::string user_guide_path;
  std::string application_prefix_url;
  std::string db_connect_string;
  int worker_count;
  int pg_pool_size;
  long maximum_request_size;
  std::vector<tile_provider> providers;
  int tile_cache_max_age;
  int tile_count_frequency;
  /**
   * The waypoint color attribute used by OsmAnd is not valid according to the
   * GPX XSD.  Setting this attribute to true allows the waypoint color
   * attribute to be included in the GPX download, otherwise it is ignored and
   * not included.  This should only matter if usage of the GPX download must
   * pass XSD validation, perhaps because of the requirements of another
   * application.
   */
  bool allow_invalid_xsd;
  /// Pretty output of XML
  bool gpx_pretty;
  /// How many spaces to indent GPX file's XML when gpx_pretty is true
  int gpx_indent;
  /**
   * This value is used to estimate the time required to hike a route, using the
   * Scarf's Equivalence algorithm.
   */
  double default_average_kmh_hiking_speed;
  int elevation_tile_cache_ms;
  std::string elevation_tile_path;
  /// The maximum number of location tracking points for related operations.
  int maximum_location_tracking_points;
  YAML::Node default_triplogger_configuration;
public:
  TripConfig(std::string filename);
  std::string get_root_directory() const {
    return root_directory;
  }
  void set_root_directory(std::string directory) {
    root_directory = directory;
  }
  std::string get_user_guide_path() const {
    return user_guide_path;
  }
  std::string get_application_prefix_url() const {
    return application_prefix_url;
  }
  std::string get_db_connect_string() const {
    return db_connect_string;
  }
  int get_worker_count() const {
    return worker_count;
  }
  int get_pg_pool_size() const {
    return pg_pool_size;
  }
  long get_maximum_request_size() const {
    return maximum_request_size;
  }
  int get_tile_cache_max_age() const {
    return tile_cache_max_age;
  }
  std::vector<tile_provider> get_providers() const {
    return providers;
  }
  int get_tile_count_frequency() const {
    return tile_count_frequency;
  }
  bool get_allow_invalid_xsd() const {
    return allow_invalid_xsd;
  }
  bool get_gpx_pretty() const {
    return gpx_pretty;
  }
  int get_gpx_indent() const {
    return gpx_indent;
  }
  double get_default_average_kmh_hiking_speed() const {
    return default_average_kmh_hiking_speed;
  }
  int get_elevation_tile_cache_ms() {
    return elevation_tile_cache_ms;
  }
  std::string get_elevation_tile_path() {
    return elevation_tile_path;
  }
  int get_maximum_location_tracking_points() {
    return maximum_location_tracking_points;
  }
  YAML::Node create_default_triplogger_configuration();
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_CONFIG_HPP
