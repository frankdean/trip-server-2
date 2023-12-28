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
#ifndef ELEVATION_TILE_HPP
#define ELEVATION_TILE_HPP

#include "itinerary_pg_dao.hpp"
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class GDALDataset;
class GDALRasterBand;
class OGRCoordinateTransformation;

namespace fdsd {
namespace trip {

class ElevationService;

/**
 * Represents a single elevation dataset containing elevations for a region.
 */
class ElevationTile {
  friend class ElevationService;
  std::string path;
  int cache_ms;
  double left;
  double pixel_width;
  double xskew;
  double top;
  double yskew;
  double pixel_height;
  double right;
  double bottom;
  GDALDataset *dataset; // not thread-safe
  GDALRasterBand *band;
  OGRCoordinateTransformation *coordinate_transform;
  std::chrono::system_clock::time_point time;
  /// mutex used to protect non-thread-safe access to dataset pointer
  std::mutex dataset_mutex;
  /// Re-opens the dataset and creates the transformation object
  void open();
  /// Deletes the dataset and the transformation object
  void close();
protected:
  static bool drivers_registered;
public:
  static const int no_data;
  ElevationTile(std::string path, int cache_ms = 60000);
  ~ElevationTile();
  std::pair<bool, double> get_elevation(double longitude, double latitude);
  class dataset_exception : public std::exception {
    std::string message;
  public:
    dataset_exception(std::string message) : std::exception(), message(message) {}
    virtual const char* what() const throw() override {
      return message.c_str();
    }
  };
};

/**
 * Manages all the ElevationTile instances and delegates calls to find an
 * elevation for a specified longitude and latitude to the relevant
 * ElevationTile instance.
 */
class ElevationService {
  std::string directory_path;
  long tile_cache_ms;
  std::vector<std::unique_ptr<ElevationTile>> tiles;
  bool initialized;
  std::exception_ptr initialization_error;
  void init();
  std::unique_ptr<std::thread> init_thread;
public:
  ElevationService(std::string directory_path, long tile_cache_ms);
  ~ElevationService();
  void update_tile_cache();
  std::pair<bool, double> get_elevation(double longitude, double latitude);

  /**
   * Iterates across the set of points, filling in elevation values where a
   * value is available.
   *
   * \param force replaces all elevation values with those in the dataset, if
   * the dataset has a value, otherwise the original value is left as-is.
   *
   * \param skip_all_if_any_exist if there is already an elevation value for one
   * or more points, no points will be updated even if force has been specified.
   *
   * \return an object containing the lowest and highest altitudes, the total
   * distance, ascent and descent.
   */
  template <typename Iterator>
  void fill_elevations(
      Iterator begin,
      Iterator end,
      bool force = false,
      bool skip_all_if_any_exist = false) {
    int elevation_count = 0;
    int points_count = 0;
    for (auto i = begin; i != end; ++i) {
      if (i->altitude.first)
        elevation_count++;
      points_count++;
    }
    // std::cout << "Total of " << points_count << " points with, "
    //           << elevation_count << " values already populated\n";

    for (auto point = begin; point != end; ++point) {
      if ((!skip_all_if_any_exist || elevation_count == 0) &&
          (elevation_count < points_count || force)) {
        // std::cout << "Trying " << point->longitude << ", "
        //           << point->latitude << '\n';

        if (!point->altitude.first || force) {
          auto altitude = get_elevation(point->longitude,
                                        point->latitude);
          // if (altitude.first) {
          //   std::cout << "Got altitude of: " << altitude.second
          //             << " for " << point->longitude << ", "
          //             << point->latitude << '\n';
          // }
          if (force) {
            if (altitude.first)
              point->altitude = altitude;
          } else {
            point->altitude = altitude;
          }
        // } else {
        //   std::cout << "Ignoring point with altitude already set\n";
        }
      }
    }
  }

  template <typename Iterator>
  void fill_elevations_for_paths(
      Iterator begin,
      Iterator end,
      bool force = false,
      bool skip_all_if_any_exist = false) {

    // This is primarily useful where the paths are all segments of the same
    // track and the statistics can be applied to the owning track on return
    path_statistics path_statistics;
    for (auto i = begin; i != end; ++i) {
      auto path = *i;
      auto points = path.points;
      fill_elevations(
          path.points.begin(),
          path.points.end(),
          force,
          skip_all_if_any_exist);
    }
  }
};

} // namespace trip
} // namespace fdsd

#endif // ELEVATION_TILE_HPP
