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
#include "geo_utils.hpp"
#include "../trip-server-common/src/get_options.hpp"
#include <iomanip>
#include <iostream>

using namespace fdsd::trip;
using namespace fdsd::utils;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

GeoMapUtils::GeoMapUtils() :
  paths(),
  min_height(std::make_pair<bool, double>(false, 0)),
  max_height(std::make_pair<bool, double>(false, 0)),
  ascent(std::make_pair<bool, double>(false, 0)),
  descent(std::make_pair<bool, double>(false, 0)),
  last_altitude(std::make_pair<bool, double>(false, 0))
{
}

std::string location::to_string() const
{
  std::ostringstream os;
  os <<
    "id: ";
  if (id.first)
    os << id.second;
  else
    os << "[null]";
  os << ", "
    "longitude: " << std::fixed << std::setprecision(6) << longitude << ", "
    "latitude: " << latitude;
  if (altitude.first) {
    os << ", "
       << "altitude: " << std::setprecision(1) << altitude.second;
  }
  return os.str();
}

YAML::Node location::encode(const location& rhs)
{
  YAML::Node node;
  if (rhs.id.first)
    node["id"] = rhs.id.second;
  else
    node["id"] = YAML::Null;
  node["lng"] = rhs.longitude;
  node["lat"] = rhs.latitude;
  if (rhs.altitude.first)
    node["altitude"] = rhs.altitude.second;
  else
    node["altitude"] = YAML::Null;
  return node;
}

bool location::decode(const YAML::Node& node, location& rhs)
{
  if ((rhs.id.first = node["id"] && !node["id"].IsNull()))
    rhs.id.second = node["id"].as<long>();
  rhs.longitude = node["lng"].as<double>();
  rhs.latitude = node["lat"].as<double>();
  if ((rhs.altitude.first = node["altitude"] && !node["altitude"].IsNull()))
    rhs.altitude.second = node["altitude"].as<double>();
  return true;
}

std::string path_statistics::to_string() const
{
  bool first = true;
  std::ostringstream os;
  if (distance.first) {
    os << "distance: " << distance.second;
    first = false;
  }
  if (ascent.first) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "ascent: " << ascent.second;
  }
  if (descent.first) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "descent: " << descent.second;
  }
  if (lowest.first) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "lowest: " << lowest.second;
  }
  if (highest.first) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "highest: " << highest.second;
  }
  return os.str();
}

YAML::Node path_statistics::encode(const path_statistics& rhs)
{
  YAML::Node node;
  if (rhs.distance.first)
    node["distance"] = rhs.distance.second;
  if (rhs.ascent.first)
    node["ascent"] = rhs.ascent.second;
  if (rhs.descent.first)
    node["descent"] = rhs.descent.second;
  if (rhs.lowest.first)
    node["lowest"] = rhs.lowest.second;
  if (rhs.highest.first)
    node["highest"] = rhs.highest.second;
  return node;
}

bool path_statistics::decode(const YAML::Node& node, path_statistics& rhs)
{
  if ((rhs.distance.first = node["distance"] && !node["distance"].IsNull()))
    rhs.distance.second = node["distance"].as<double>();
  if ((rhs.ascent.first = node["ascent"] && !node["ascent"].IsNull()))
    rhs.ascent.second = node["ascent"].as<double>();
  if ((rhs.descent.first = node["descent"] && !node["descent"].IsNull()))
    rhs.descent.second = node["descent"].as<double>();
  if ((rhs.lowest.first =  node["lowest"] && !node["lowest"].IsNull()))
    rhs.lowest.second = node["lowest"].as<double>();
  if ((rhs.highest.first = node["highest"] && !node["highest"].IsNull()))
    rhs.highest.second = node["highest"].as<double>();
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
  if (last_altitude.first && loc->altitude.first) {
    // See the GeoStatistics::upudate_statistics method
    double diff = loc->altitude.second - last_altitude.second;
    // std::cout << std::fixed << std::setprecision(1) << "Diff: " << diff << '\n';
    if (diff > 0) {
      if (ascent.first) {
        ascent.second += diff;
      } else {
        ascent.first = true;
        ascent.second = diff;
      }
    } else {
      if (descent.first) {
        descent.second -= diff;
      } else {
        descent.first = true;
        descent.second = -diff;
      }
    }
    last_altitude.second = loc->altitude.second;
  } else {
    last_altitude = loc->altitude;
  }
  // std::cout << "Last height: " << (last_altitude.first ? last_altitude.second : 0) << '\n'
  //           << "Ascent:  " << (ascent.first ? ascent.second : 0) << '\n'
  //           << "Descent: " << (descent.first ? descent.second : 0) << '\n';

  if (max_height.first) {
    max_height.second = std::max(max_height.second, loc->altitude.second);
  } else {
    max_height = loc->altitude;
  }
  if (min_height.first) {
    min_height.second = std::min(min_height.second, loc->altitude.second);
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
  // TODO calculate actual position line intersects the anti-meridian It is not
  // as simple as breaking the line at longitude 180° and copying the same
  // latitude values, which only works with a line parallel to a latitude.
  update_altitude_info(&loc);
  if (previous) {
    if (previous->longitude > 90 && loc.longitude < -90 ||
        previous->longitude < -90 && loc.longitude > 90) {
      // std::cout << "Path from location id ";
      // if (previous->id.first)
      //   std::cout << previous->id.second;
      // else
      //   std::cout << "null";
      // std::cout << " to location id ";
      // if (loc.id.first)
      //   std::cout << loc.id.second;
      // else
      //   std::cout << "null"
      //             << " crosses the anti-meridian\n";
      location t1(loc);
      location t2(loc);
      // std::cout << "First: " << t1 << '\n';
      // std::cout << "Second: " << t2 << '\n';
      if (t2.longitude < t1.longitude) {
        t1.longitude = -180.0;
        t2.longitude = 180.0;
      } else {
        t1.longitude = 180.0;
        t2.longitude = -180.0;
      }
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
    if (GetOptions::verbose_flag)
      std::cerr << "WARNING: Created a MultiLineString GeoJSON feature which this application doesn't currently handle\n";
    json json_paths = json::array();
    for (const auto &path : paths) {
      json coords = json::array();
      for (const auto &loc : path) {
        json coord;
        coord.push_back(loc.longitude);
        coord.push_back(loc.latitude);
        if (loc.altitude.first)
          coord.push_back(loc.altitude.second);
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
    for (const auto &loc : path) {
      json coord;
      coord.push_back(loc.longitude);
      coord.push_back(loc.latitude);
      if (loc.altitude.first)
        coord.push_back(loc.altitude.second);
      coords.push_back(coord);
    }
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

double GeoUtils::degrees_to_radians(double d)
{
    return d * pi / 180;
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

double GeoUtils::distance(location p1, location p2)
{
  return distance(p1.longitude, p1.latitude, p2.longitude, p2.latitude);
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
    std::pair<bool, double> &local_last_altitude,
    location &loc)
{
  update_statistics(local_stats, local_last_location, local_last_altitude, loc);
  // std::cout << "After add_location: ";
  // if (local_last_altitude.first)
  //   std::cout << " last altitude: " << local_last_altitude.second;
  // std::cout << '\n';
  update_statistics(*this, last_location, last_altitude, loc);
}

void GeoStatistics::update_statistics(
    path_statistics &statistics,
    std::unique_ptr<location> &local_last_location,
    std::pair<bool, double> &local_last_altitude,
    location &loc)
{
  // std::cout << "Location: " << *loc << '\n';
  if (local_last_location) {
    // std::cout << "Last location: " << *local_last_location << '\n';
    const double leg_distance = GeoUtils::distance(*local_last_location, loc);
    if (statistics.distance.first) {
      statistics.distance.second += leg_distance;
      // std::cout << "Added leg distance: " << leg_distance << '\n';
    } else {
      statistics.distance.first = true;
      // std::cout << "Set leg distance: " << leg_distance << '\n';
      statistics.distance.second = leg_distance;
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

  if (loc.altitude.first) {
    if (local_last_altitude.first) {
      // std::cout << "Previous last altitude: " << local_last_altitude.second << '\n';
      const double altitude_change =
        loc.altitude.second - local_last_altitude.second;
      // std::cout << "Altitude change: " << altitude_change << '\n';

      if (altitude_change > 0) {
        if (statistics.ascent.first) {
          // std::cout << "Updating ascent to " << altitude_change << '\n';
          statistics.ascent.second += altitude_change;
        } else {
          // std::cout << "Setting first ascent to " << altitude_change << '\n';
          statistics.ascent.first = true;
          statistics.ascent.second = altitude_change;
        }
      } else if (altitude_change < 0) {
        if (statistics.descent.first) {
          statistics.descent.second += std::abs(altitude_change);
        } else {
          statistics.descent.first = true;
          statistics.descent.second = std::abs(altitude_change);
        }
      }
    }
    local_last_altitude = loc.altitude;
    // if (loc.altitude.first)
    //   std::cout << "Saving last altitude as: " << loc.altitude.second << '\n';

    if (statistics.lowest.first) {
      statistics.lowest.second =
        std::min(statistics.lowest.second, loc.altitude.second);
    } else {
      statistics.lowest = loc.altitude;
    }
    if (statistics.highest.first) {
      statistics.highest.second =
        std::max(statistics.highest.second, loc.altitude.second);
    } else {
      statistics.highest = loc.altitude;
    }
  // } else {
  //   std::cout << "Altitude not set\n";
  }

}
