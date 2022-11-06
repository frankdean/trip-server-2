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
#ifndef GEO_UTILS_HPP
#define  GEO_UTILS_HPP

#include <chrono>
#include <vector>
#include <sstream>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

namespace fdsd {
namespace trip {

const double kms_per_mile = 1.609344;
const double feet_per_meter = 0.3048;

struct location {
  long id;
  double longitude;
  double latitude;
  // std::optional<double> altitude;
  std::pair<bool, double> altitude;
  location() {}
  location(long id,
           double lon,
           double lat,
           std::pair<bool, double> altitude = std::pair<bool, double>(false, 0));
  virtual std::string to_string() const;
  inline friend std::ostream& operator<<
      (std::ostream& out, const location& rhs) {
    return out << rhs.to_string();
  }
};

/**
 * Used to build a list of related paths, that may consitute a single path, or
 * multiple sections of a path.  It handles paths that cross the anti-meridian
 * by splitting the path into seperate paths such that they join at the
 * anti-meridian, but do not cross it.  \see GeoUtils#add_path method for more
 * detail.
 *
 * It is used to create a GeoJSON representation of the path.
 */
class GeoUtils {
  std::vector<std::vector<location>> paths;
  std::pair<bool, double> min_height;
  std::pair<bool, double> max_height;
  std::pair<bool, double> ascent;
  std::pair<bool, double> descent;
  /// The height of the last coordinate containing an altitude value
  std::pair<bool, double> last_height;
  void update_altitude_info(const location& loc);
  void add_location(std::pair<bool, location> &last,
                    std::vector<location> &new_path,
                    const location& loc);
public:
  GeoUtils();
  nlohmann::basic_json<nlohmann::ordered_map> as_geojson(const int indent = -1,
                  const char indent_char = ' ') const;

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
    std::pair<bool, location> last = std::make_pair(false, location());
    std::vector<location> new_path;
    while (begin != end) {
      location &loc = *begin;
      add_location(last, new_path, loc);
      ++ begin;
    }
    if (!new_path.empty())
      paths.push_back(new_path);
  }

std::pair<bool, double> get_min_height() {
    return min_height;
  }
  std::pair<bool, double> get_max_height() {
    return max_height;
  }
  std::pair<bool, double> get_ascent() {
    return ascent;
  }
  std::pair<bool, double> get_descent() {
    return descent;
  }
};

} // namespace trip
} // namespace fdsd

#endif //  GEO_UTILS_HPP
