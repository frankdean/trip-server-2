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
#ifndef GEO_UTILS_HPP
#define  GEO_UTILS_HPP

#include <chrono>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

namespace fdsd {
namespace trip {

const double kms_per_mile = 1.609344;
const double inches_per_meter = 0.0254;
const double feet_per_meter = 0.3048;
/// Mean radius of the Earth in kilometers
const double earth_mean_radius_kms = 6371; // https://en.wikipedia.org/wiki/Earth
const double pi = 3.141592653589793;

struct location {
  std::optional<long> id;
  double longitude;
  double latitude;
  // std::optional<double> altitude;
  std::optional<double> altitude;
  location() : id(),
               longitude(),
               latitude(),
               altitude() {}
  location(double lon,
           double lat,
           std::optional<double> altitude = std::optional<double>())
    : id(),
      longitude(lon),
      latitude(lat),
      altitude(altitude) {}
  location(long id,
           double lon,
           double lat,
           std::optional<double> altitude = std::optional<double>())
    : id(id),
      longitude(lon),
      latitude(lat),
      altitude(altitude) {}
  virtual ~location() {}
  virtual std::string to_string() const;
  inline friend std::ostream& operator<<
      (std::ostream& out, const location& rhs) {
    return out << rhs.to_string();
  }
  static YAML::Node encode(const location& rhs);
  static bool decode(const YAML::Node& node, location& rhs);
};

struct path_statistics {
  std::optional<double> distance;
  std::optional<double> ascent;
  std::optional<double> descent;
  std::optional<double> lowest;
  std::optional<double> highest;
  path_statistics() :
    distance(),
    ascent(),
    descent(),
    lowest(),
    highest() {}
  virtual ~path_statistics() {}
  virtual std::string to_string() const;
  inline friend std::ostream& operator<<
      (std::ostream& out, const path_statistics& rhs) {
    return out << rhs.to_string();
  }
  static YAML::Node encode(const path_statistics& rhs);
  static bool decode(const YAML::Node& node, path_statistics& rhs);
};

/**
 * Used to build a list of related paths, that may constitute a single path, or
 * multiple sections of a path.  It handles paths that cross the anti-meridian
 * by splitting the path into separate paths such that they join at the
 * anti-meridian, but do not cross it.  \see GeoMapUtils#add_path method for more
 * detail.
 *
 * It is used to create a GeoJSON representation of the path.
 */
class GeoMapUtils {
  std::vector<std::vector<location>> paths;
  std::optional<double> min_height;
  std::optional<double> max_height;
  std::optional<double> ascent;
  std::optional<double> descent;
  /// The height of the last coordinate containing an altitude value
  std::optional<double> last_altitude;
  void update_altitude_info(const location *loc);
  void add_location(std::unique_ptr<location> &previous,
                    std::vector<location> &new_path,
                    location &loc);
public:
  GeoMapUtils();
  nlohmann::basic_json<nlohmann::ordered_map> as_geojson() const;

  /**
   * Adds the passed path to the list of paths.  Any coordinates that cross the
   * anti-meridian (180° longitude) are split into multiple paths.  So a path
   * containing one such coordinate will result in two paths being created, a
   * path with two such coordinates will create three paths, etc.  This is to
   * comply with section 3.1.9 (Antimeridian Cutting) of RFC 7946 [1].
   *
   * [1]: Butler, H., Daly, M., Doyle, A., Gillies, S., Hagen, S., and
   * T. Schaub, "The GeoJSON Format", RFC 7946, DOI 10.17487/RFC7946,
   * August 2016, <https://www.rfc-editor.org/info/rfc7946>.
   *
   * \param begin The begin iterator of the path the path to be added.
   * \param end The end iterator of the path the path to be added.
   */
  template <typename Iterator>
  void add_path(Iterator begin, Iterator end){
    std::unique_ptr<location> previous;
    std::vector<location> new_path;
    while (begin != end) {
      location loc = *begin;
      add_location(previous, new_path, loc);
      ++begin;
    }
    if (!new_path.empty())
      paths.push_back(new_path);
  }

  std::optional<double> get_min_height() {
    return min_height;
  }
  std::optional<double> get_max_height() {
    return max_height;
  }
  std::optional<double> get_ascent() {
    return ascent;
  }
  std::optional<double> get_descent() {
    return descent;
  }
};

struct bounding_box {
  location top_left;
  location bottom_right;
  bounding_box() : top_left(), bottom_right() {}
  bounding_box(location loc) : top_left(loc), bottom_right(loc) {}
  virtual ~bounding_box() {}
  void extend(const location &location);
  location get_center() const;
  virtual std::string to_string() const;
  inline friend std::ostream& operator<<
      (std::ostream& out, const bounding_box& rhs) {
    return out << rhs.to_string();
  }
};

class GeoUtils {
public:
  static double degrees_to_radians(double d);
  static double radians_to_degrees(double r);
  static double haversine(double angle);
  static double distance(double lng1, double lat1, double lng2, double lat2);
  static double distance(const location &p1, const location &p2);
  static double bearing_to_azimuth(double lng1, double lat1, double lng2, double lat2);
  static double bearing_to_azimuth(const location &p1, const location &p2) {
    return bearing_to_azimuth(p1.longitude, p1.latitude, p2.longitude, p2.latitude);
  }
};

class GeoStatistics : path_statistics {
  std::unique_ptr<location> last_location;
  std::optional<double> last_altitude;
public:
  GeoStatistics() : path_statistics(),
                    last_location(nullptr),
                    last_altitude() {
    distance = 0;
  }

  virtual std::string to_string() const;
  inline friend std::ostream& operator<<
      (std::ostream& out, const GeoStatistics& rhs) {
    return out << rhs.to_string();
  }

  static void update_statistics(path_statistics &statistics,
                                std::unique_ptr<location> &local_last_location,
                                std::optional<double> &local_last_altitude,
                                location &loc);

  void add_location(
      path_statistics &local_stats,
      std::unique_ptr<location> &local_last_location,
      std::optional<double> &local_last_altitude,
      location &loc);

  template <typename Iterator>
  path_statistics add_path(Iterator begin, Iterator end) {
    path_statistics retval;
    std::unique_ptr<location> local_last_location = nullptr;
    std::optional<double> local_last_altitude;
    for (auto i = begin; i != end; ++i) {
      location location = *i;
      add_location(retval, local_last_location, local_last_altitude, location);

      // std::cout << ++debug_count << ": Add path, after add_location: ";
      // if (local_last_altitude.has_value())
      //   std::cout << " last altitude: " << local_last_altitude.value();
      // std::cout << '\n';
    }
    return retval;
  }
  std::optional<double> get_lowest() {
    return lowest;
  }
  std::optional<double> get_highest() {
    return highest;
  }
  std::optional<double> get_ascent() {
    return ascent;
  }
  std::optional<double> get_descent() {
    return descent;
  }
  std::optional<double> get_distance() {
    return distance;
  }
};

} // namespace trip
} // namespace fdsd

#endif //  GEO_UTILS_HPP
