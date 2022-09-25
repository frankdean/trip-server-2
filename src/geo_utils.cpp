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
#include "geo_utils.hpp"
#include <iomanip>
#include <iostream>

using namespace fdsd::trip;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

location::location(long long id,
                   double lon,
                   double lat,
                   std::pair<bool, double> altitude)
{
  this->id = id;
  this->longitude = lon;
  this->latitude = lat;
  this->altitude = altitude;
}

GeoUtils::GeoUtils() :
  paths(),
  min_height(std::make_pair<bool, double>(false, 0)),
  max_height(std::make_pair<bool, double>(false, 0)),
  ascent(std::make_pair<bool, double>(false, 0)),
  descent(std::make_pair<bool, double>(false, 0)),
  last_height(std::make_pair<bool, double>(false, 0))
{
}

std::string location::to_string() const
{
  std::ostringstream os;
  os <<
    "id: " << id << ", "
    "longitude: " << std::fixed << std::setprecision(6) << longitude << ", "
    "latitude: " << latitude;
  if (altitude.first) {
    os << ", "
       << "altitude: " << std::setprecision(1) << altitude.second;
  }
  return os.str();
}

/**
 * Updates the various altitude related member variables using the passed
 * location.
 *
 * \param the location to update details for.
 */
void GeoUtils::update_altitude_info(const location& loc)
{
  if (last_height.first) {
    double diff = loc.altitude.second - last_height.second;
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
    last_height.second = loc.altitude.second;
  } else {
    last_height = loc.altitude;
  }
  // std::cout << "Last height: " << (last_height.first ? last_height.second : 0) << '\n'
  //           << "Ascent:  " << (ascent.first ? ascent.second : 0) << '\n'
  //           << "Descent: " << (descent.first ? descent.second : 0) << '\n';

  if (max_height.first) {
    max_height.second = std::max(max_height.second, loc.altitude.second);
  } else {
    max_height = loc.altitude;
  }
  if (min_height.first) {
    min_height.second = std::min(min_height.second, loc.altitude.second);
  } else {
    min_height = loc.altitude;
  }
}

/**
 * Adds the passed location to the passed path.  If the path needs splitting,
 * the first section of the split path is added to the internal list of paths.
 *
 * \param last a std:pair, updated to hold the last location processed.  The
 * first element of the pair is true after the value has been updated.
 *
 * \param a temporary list of locations which will eventually be added to the
 * internal list.
 *
 * \param the current location being analyzed.
 */
void GeoUtils::add_location(std::pair<bool, location> &last,
                            std::vector<location> &new_path,
                            const location& loc)
{
  update_altitude_info(loc);
  if (last.first) {
    if (last.second.longitude > 90 && loc.longitude < -90 ||
        last.second.longitude < -90 && loc.longitude > 90) {
      // std::cout << "Path from location id "
      //           << last.second.id << " to location id "
      //           << loc.id
      //           << " crosses the anti-meridian\n";
      auto t1 = loc;
      auto t2 = loc;
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
      last = std::make_pair(true, t2);
    } else {
      new_path.push_back(loc);
      last = std::make_pair(true, loc);
    }
  } else {
    new_path.push_back(loc);
    last = std::make_pair(true, loc);
  }
}

/**
 * \return a GeoJSON representation of the current path.  If there are
 * multiple paths, a MultiLineString is returned.  If there is only a single
 * path, a LineString is returned.  For each path that contains only one
 * point, a Point is returned.
 */
nlohmann::basic_json<nlohmann::ordered_map> GeoUtils::as_geojson(const int indent,
  const char indent_char) const
{
  if (paths.size() > 1) {
    json json_paths = json::array();
    for (const auto &path : paths) {
      json coords = json::array();
      for (const auto &loc : path) {
        json coord;
        coord.push_back(std::round(loc.longitude * 1e+06) / 1e+06);
        coord.push_back(std::round(loc.latitude * 1e+06) / 1e+06);
        if (loc.altitude.first)
          coord.push_back(std::round(loc.altitude.second * 10) / 10);
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
      coord.push_back(std::round(loc.longitude * 1e+06) / 1e+06);
      coord.push_back(std::round(loc.latitude * 1e+06) / 1e+06);
      if (loc.altitude.first)
        coord.push_back(std::round(loc.altitude.second * 10) / 10);
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
