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
#include "geo_utils.hpp"
#include "../trip-server-common/src/get_options.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace fdsd::trip;
using namespace fdsd::utils;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

GeoMapUtils::GeoMapUtils() :
  paths(),
  min_height(),
  max_height(),
  ascent(),
  descent(),
  last_altitude()
{
}

std::string location::to_string() const
{
  std::ostringstream os;
  os <<
    "id: ";
  if (id.has_value())
    os << id.value();
  else
    os << "[null]";
  os << ", "
    "longitude: " << std::fixed << std::setprecision(6) << longitude << ", "
    "latitude: " << latitude;
  if (altitude.has_value()) {
    os << ", "
       << "altitude: " << std::setprecision(1) << altitude.value();
  }
  return os.str();
}

YAML::Node location::encode(const location& rhs)
{
  YAML::Node node;
  if (rhs.id.has_value())
    node["id"] = rhs.id.value();
  else
    node["id"] = YAML::Null;
  node["lng"] = rhs.longitude;
  node["lat"] = rhs.latitude;
  if (rhs.altitude.has_value())
    node["altitude"] = rhs.altitude.value();
  else
    node["altitude"] = YAML::Null;
  return node;
}

bool location::decode(const YAML::Node& node, location& rhs)
{
  if (node["id"] && !node["id"].IsNull())
    rhs.id = node["id"].as<long>();
  rhs.longitude = node["lng"].as<double>();
  rhs.latitude = node["lat"].as<double>();
  if (node["altitude"] && !node["altitude"].IsNull())
    rhs.altitude = node["altitude"].as<double>();
  return true;
}

std::string path_statistics::to_string() const
{
  bool first = true;
  std::ostringstream os;
  if (distance.has_value()) {
    os << "distance: " << distance.value();
    first = false;
  }
  if (ascent.has_value()) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "ascent: " << ascent.value();
  }
  if (descent.has_value()) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "descent: " << descent.value();
  }
  if (lowest.has_value()) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "lowest: " << lowest.value();
  }
  if (highest.has_value()) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "highest: " << highest.value();
  }
  return os.str();
}

YAML::Node path_statistics::encode(const path_statistics& rhs)
{
  YAML::Node node;
  if (rhs.distance.has_value())
    node["distance"] = rhs.distance.value();
  if (rhs.ascent.has_value())
    node["ascent"] = rhs.ascent.value();
  if (rhs.descent.has_value())
    node["descent"] = rhs.descent.value();
  if (rhs.lowest.has_value())
    node["lowest"] = rhs.lowest.value();
  if (rhs.highest.has_value())
    node["highest"] = rhs.highest.value();
  return node;
}

bool path_statistics::decode(const YAML::Node& node, path_statistics& rhs)
{
  if (node["distance"] && !node["distance"].IsNull())
    rhs.distance = node["distance"].as<double>();
  if (node["ascent"] && !node["ascent"].IsNull())
    rhs.ascent = node["ascent"].as<double>();
  if (node["descent"] && !node["descent"].IsNull())
    rhs.descent = node["descent"].as<double>();
  if (node["lowest"] && !node["lowest"].IsNull())
    rhs.lowest = node["lowest"].as<double>();
  if (node["highest"] && !node["highest"].IsNull())
    rhs.highest = node["highest"].as<double>();
  return true;
}

/**
 * Updates the various altitude related member variables using the passed
 * location.
 *
 * \param the location to update details from.
 */
void GeoMapUtils::update_altitude_info(const location *loc)
{
  if (!loc->altitude.has_value())
    return;
  if (last_altitude.has_value()) {
    // See the GeoStatistics::upudate_statistics method
    double diff = loc->altitude.value() - last_altitude.value();
    // std::cout << std::fixed << std::setprecision(1) << "Diff: " << diff << '\n';
    if (diff > 0) {
      if (ascent.has_value()) {
        ascent = ascent.value() + diff;
      } else {
        ascent = diff;
      }
    } else {
      if (descent.has_value()) {
        descent = descent.value() - diff;
      } else {
        descent = -diff;
      }
    }
    last_altitude = loc->altitude;
  } else {
    last_altitude = loc->altitude;
  }
  // std::cout << "Last height: " << (last_altitude.first ? last_altitude.second : 0) << '\n'
  //           << "Ascent:  " << (ascent.first ? ascent.second : 0) << '\n'
  //           << "Descent: " << (descent.first ? descent.second : 0) << '\n';

  if (max_height.has_value()) {
    max_height = std::max(max_height.value(), loc->altitude.value());
  } else {
    max_height = loc->altitude;
  }
  if (min_height.has_value()) {
    min_height = std::min(min_height.value(), loc->altitude.value());
  } else {
    min_height = loc->altitude;
  }
}

/**
 * Adds the passed location to the passed path.  If the path needs splitting,
 * the first section of the split path is added to the internal list of paths.
 *
 * \param previous a std:pair, updated to hold the previous location processed.
 * The first element of the pair is true after the value has been updated.
 *
 * \param a temporary list of locations which will eventually be added to the
 * internal list.
 *
 * \param the current location being analyzed.
 */
void GeoMapUtils::add_location(std::unique_ptr<location> &previous,
                               std::vector<location> &new_path,
                               location &loc)
{
  update_altitude_info(&loc);
  if (previous) {
    if (previous->longitude > 90 && loc.longitude < -90 ||
        previous->longitude < -90 && loc.longitude > 90) {
      // Paths that cross the antimeridian pose problems when displayed on the
      // map.  We try to calculate the actual position the path intersects the
      // antimeridian and create extra points at roughly where the intersection
      // occurs.  We then split the path into two.  This is not accurate, but
      // perhaps good enough for most situations.
      //
      // See the 'Antimeridian Cutting' section of the GeoJSON specification.
      // https://datatracker.ietf.org/doc/html/rfc7946
      location t1(loc);
      location t2(loc);
      if (previous->longitude < 0) {
        t1.longitude = -180.0;
        t2.longitude = 180.0;
      } else {
        t1.longitude = 180.0;
        t2.longitude = -180.0;
      }
      const auto ydiff = loc.latitude - previous->latitude;
      t1.latitude -= (ydiff / 2);
      t2.latitude = t1.latitude;
      update_altitude_info(&t1);
      t2.altitude = t1.altitude;
      new_path.push_back(t1);
      paths.push_back(new_path);
      new_path = std::vector<location>();
      new_path.push_back(t2);
      new_path.push_back(loc);
      previous = std::unique_ptr<location>(new location(t2));
    } else {
      new_path.push_back(location(loc));
      previous = std::unique_ptr<location>(new location(loc));
    }
  } else {
    new_path.push_back(loc);
    previous = std::unique_ptr<location>(new location(loc));
  }
}

/**
 * \return a GeoJSON representation of the current path.  If there are
 * multiple paths, a MultiLineString is returned.  If there is only a single
 * path, a LineString is returned.  For each path that contains only one
 * point, a Point is returned.
 */
nlohmann::basic_json<nlohmann::ordered_map> GeoMapUtils::as_geojson(const int indent,
  const char indent_char) const
{
  if (paths.size() > 1) {
    json json_paths = json::array();
    for (const auto &path : paths) {
      json coords = json::array();
      for (const auto &loc : path) {
        json coord;
        coord.push_back(loc.longitude);
        coord.push_back(loc.latitude);
        if (loc.altitude.has_value())
          coord.push_back(loc.altitude.value());
        coords.push_back(coord);
      }
      json_paths.push_back(coords);
    }
    json geometry {
      {"type", "MultiLineString"},
      {"coordinates", json_paths}
    };
    return geometry;
  } else if (paths.empty()) {
    return json();
  } else {
    auto path = paths.front();
    json coords = json::array();
    // OpenLayers 9.x needs consistent coordinate array length throughout,
    // either all length 2 or all length 3.
    bool contains_elevation_data = false;
    bool contains_null_elevation_data = false;
    for (const auto &loc : path) {
      json coord;
      coord.push_back(loc.longitude);
      coord.push_back(loc.latitude);
      if (loc.altitude.has_value()) {
        coord.push_back(loc.altitude.value());
        contains_elevation_data = true;
      } else {
        if (contains_elevation_data)
          coord.push_back(nullptr);
        else
          contains_null_elevation_data = true;
      }
      coords.push_back(coord);
    }
    // Fix the array if some entries have elevation data (length 3) and some do
    // not (length 2)
    if (contains_elevation_data && contains_null_elevation_data)
      for (auto &coord : coords)
        if (coord.size() == 2)
          coord.push_back(nullptr);
    json geometry;
    if (coords.empty()) {
      return "{}";
    } else if (coords.size() == 1) {
      geometry["type"] = "Point";
      geometry["coordinates"] = coords.front();
    } else {
      geometry["type"] = "LineString";
      geometry["coordinates"] = coords;
    }
    return geometry;
  }
}

void bounding_box::extend(const location &location)
{
  auto left = std::min(top_left.longitude, location.longitude);
  auto right = std::max(bottom_right.longitude, location.longitude);
  auto top = std::max(top_left.latitude, location.latitude);
  auto bottom = std::min(bottom_right.latitude, location.latitude);
  top_left.longitude = left;
  top_left.latitude = top;
  bottom_right.longitude = right;
  bottom_right.latitude = bottom;
}

location bounding_box::get_center() const
{
  location loc;
  auto width =  bottom_right.longitude - top_left.longitude;
  auto height = top_left.latitude - bottom_right.latitude;
  loc.longitude = top_left.longitude + width / 2;
  loc.latitude = bottom_right.latitude + height / 2;
  return loc;
}

std::string bounding_box::to_string() const
{
  std::ostringstream os;
  os << "top_left: " << top_left
     << ", bottom_right: " << bottom_right;
  return os.str();
}

double GeoUtils::degrees_to_radians(double d)
{
    return d * pi / 180;
}

double GeoUtils::radians_to_degrees(double r)
{
    return r * 180 / pi;
}

double GeoUtils::haversine(double angle)
{
  // Two possible implementations
  return std::pow(std::sin(angle / 2), 2);
  // return (1 - std::cos(angle)) / 2;
}

//// Calculates the distance between two lat/lng points using the Haversine
//// Formula.  The units for the return value will be the same as that used
//// within the calculation for the Earth's mean radius, which should be in
//// kilometers.
double GeoUtils::distance(double lng1, double lat1, double lng2, double lat2)
{
  // std::cout
  //   << "Calculating distance between lat: " << lat1 << " lng: " << lng1
  //   << " and lat: " << lat2 << " lng: " << lng2 << '\n';

  double x1 = degrees_to_radians(lng1);
  double y1 = degrees_to_radians(lat1);
  double x2 = degrees_to_radians(lng2);
  double y2 = degrees_to_radians(lat2);

  // https://en.wikipedia.org/wiki/Haversine_formula
  return std::asin(
      std::sqrt(
          haversine(y2 - y1)
          + (1 - haversine(y1 - y2) - haversine(y1 + y2))
          * haversine(x2 - x1)
        )
    )
    * 2 * earth_mean_radius_kms;

  // An alternative implementation
  // return std::asin(std::sqrt(haversine(y2 - y1)
  //                            + std::cos(y1)
  //                            * std::cos(y2)
  //                            * haversine((x2 - x1))))
  //   * 2 * earth_mean_radius_kms;

}

double GeoUtils::distance(const location &p1, const location &p2)
{
  return distance(p1.longitude, p1.latitude, p2.longitude, p2.latitude);
}

double GeoUtils::bearing_to_azimuth(
    double lng1, double lat1, double lng2, double lat2)
{
  // https://mapscaping.com/how-to-calculate-bearing-between-two-coordinates/
  // https://www.igismap.com/formula-to-find-bearing-or-heading-angle-between-two-points-latitude-longitude/
  const auto delta = degrees_to_radians(lng2 - lng1);
  const auto lat1_r = degrees_to_radians(lat1);
  const auto lat2_r = degrees_to_radians(lat2);
  const auto x = std::cos(lat2_r) * std::sin(delta);
  const auto y = std::cos(lat1_r) * std::sin(lat2_r) - std::sin(lat1_r) *
    std::cos(lat2_r) * std::cos(delta);
  auto degrees = radians_to_degrees(std::atan2(x, y));
  if (degrees < 0)
    degrees += 360;
  return degrees;
}

std::string GeoStatistics::to_string() const
{
  std::stringstream os;
  os << path_statistics::to_string();
  os << ", last_location: " << *last_location;
  if (last_altitude.has_value())
    os << ", last_altitude: " << last_altitude.value();
  return os.str();
}

/**
 *
 * \param local_stats statistics to update separately.  These will typically be
 * used to get separate figures for each segment, which will have it's set of
 * statistics, separate to the parent's cumulative statistics.
 * \param location the location to be included in the statistcis.
 */
void GeoStatistics::add_location(
    path_statistics &local_stats,
    std::unique_ptr<location> &local_last_location,
    std::optional<double> &local_last_altitude,
    location &loc)
{
  update_statistics(local_stats, local_last_location, local_last_altitude, loc);
  // std::cout << "After add_location: ";
  // if (local_last_altitude.has_value())
  //   std::cout << " last altitude: " << local_last_altitude.value();
  // std::cout << '\n';
  update_statistics(*this, last_location, last_altitude, loc);
}

void GeoStatistics::update_statistics(
    path_statistics &statistics,
    std::unique_ptr<location> &local_last_location,
    std::optional<double> &local_last_altitude,
    location &loc)
{
  // std::cout << "\nUpdating statistics for location: " << loc << '\n';
  if (local_last_location) {
    // std::cout << "Last location: " << *local_last_location << '\n';
    const double leg_distance = GeoUtils::distance(*local_last_location, loc);
    if (statistics.distance.has_value()) {
      statistics.distance = statistics.distance.value() + leg_distance;
      // std::cout << "Added leg distance: " << leg_distance << '\n';
    } else {
      // std::cout << "Set leg distance: " << leg_distance << '\n';
      statistics.distance = leg_distance;
    }
  // } else {
  //   std::cout << "Fresh leg\n";
  }
  // if (statistics.distance.first)
  //   std::cout << "sub total: " << statistics.distance.second << '\n';
  // else {
  //   std::cout << "No distance value\n";
  // }
  local_last_location = std::unique_ptr<location>(new location(loc));

  if (loc.altitude.has_value()) {
    if (local_last_altitude.has_value()) {
      // std::cout << "Previous last altitude: " << local_last_altitude.value() << '\n';
      const double altitude_change =
        loc.altitude.value() - local_last_altitude.value();
      // std::cout << "Altitude change: " << altitude_change << '\n';

      if (altitude_change > 0) {
        if (statistics.ascent.has_value()) {
          // std::cout << "Updating ascent by " << altitude_change << '\n';
          statistics.ascent = statistics.ascent.value() + altitude_change;
        } else {
          // std::cout << "Setting first ascent to " << altitude_change << '\n';
          statistics.ascent = altitude_change;
        }
      } else if (altitude_change < 0) {
        if (statistics.descent.has_value()) {
          statistics.descent = statistics.descent.value() + std::abs(altitude_change);
        } else {
          statistics.descent = std::abs(altitude_change);
        }
      }
    }
    local_last_altitude = loc.altitude;
    // if (local_last_altitude.has_value()) {
    //   std::cout << "Saved last altitude as: " << local_last_altitude.value() << '\n';
    // } else {
    //   std::cout << "Saved last altitude as: [null]\n";
    // }

    if (statistics.lowest.has_value()) {
      statistics.lowest =
        std::min(statistics.lowest.value(), loc.altitude.value());
    } else {
      statistics.lowest = loc.altitude;
    }
    if (statistics.highest.has_value()) {
      statistics.highest =
        std::max(statistics.highest.value(), loc.altitude.value());
    } else {
      statistics.highest = loc.altitude;
    }
  // } else {
  //   std::cout << "Altitude not set\n";
  }

}
