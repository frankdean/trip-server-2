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
#ifndef ELEVATION_TILE_HPP
#define ELEVATION_TILE_HPP

#ifdef HAVE_GDAL

#include <algorithm>
#include <chrono>
#include <functional>
#include <optional>
#include <stdexcept>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <set>
#include <string>
#ifdef HAVE_THREAD
#include <thread>
#endif
#ifdef HAVE_NLOHMANN_JSON_HPP
#include <nlohmann/json.hpp>
#endif
#ifdef HAVE_TARGETCONDITIONALS_H
#include <TargetConditionals.h>
#endif

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
  enum geo_tiff_file_types {
    unknown,
    tar,
    tgz,
    tiff,
    zip
  };
  std::string filename;
  double top;
  double right;
  double bottom;
  double left;
  double xskew;
  double yskew;
  double pixel_width;
  double pixel_height;
  GDALDataset *dataset; // not thread-safe
  GDALRasterBand *band;
  OGRCoordinateTransformation *coordinate_transform;
  std::chrono::system_clock::time_point time;
  /// mutex used to protect non-thread-safe access to dataset pointer
  static std::mutex dataset_mutex;
  /// Re-opens the dataset and creates the transformation object
  void open(std::string directory_path);
  /// Deletes the dataset and the transformation object
  void close();
  static std::string build_gdal_path_name(
      std::string directory_path,
      std::string filename);
  static ElevationTile::geo_tiff_file_types get_geo_tiff_tile_type(const std::string filename);
  std::optional<double> get_elevation(std::string directory_path,
                                      double longitude,
                                      double latitude);
  static std::string strip_file_extension(std::string filename);
#ifdef HAVE_NLOHMANN_JSON_HPP
  ElevationTile(nlohmann::basic_json<nlohmann::ordered_map> j);
#endif
protected:
  static bool drivers_registered;
public:
  /// Constant indicating the coordinate has no elevation data
  static const double no_data;
  /// Constructs q new tile, loading it from the specified path
  ElevationTile(std::string directory_path, std::string filename);
  ~ElevationTile() {}
#ifdef HAVE_NLOHMANN_JSON_HPP
  static void from_json(const nlohmann::basic_json<nlohmann::ordered_map>& j, ElevationTile& t);
  static void to_json(nlohmann::basic_json<nlohmann::ordered_map>& j, const ElevationTile& t);
#endif
  class dataset_exception : public std::exception {
    std::string message;
  public:
    dataset_exception(std::string message) : std::exception(), message(message) {}
    virtual const char* what() const throw() override {
      return message.c_str();
    }
  };
};

#ifdef TARGET_OS_MACCATALYST
typedef std::vector<ElevationTile> map_tile_type;
#else
typedef std::map<std::string, ElevationTile> map_tile_type;
#endif

/**
 * Manages all the ElevationTile instances and delegates calls to find an
 * elevation for a specified longitude and latitude to the relevant
 * ElevationTile instance.
 */
class ElevationService {
  std::string directory_path;
  std::string index_pathname;
  long tile_cache_ms;
  /// Map keyed by filename (without full path)
  map_tile_type tiles;
  bool initialized;
  std::exception_ptr initialization_error;
  void init();
  /// @param filename (without path)
  std::string build_gdal_path_name(std::string filename) const;
#ifdef HAVE_THREAD
  std::unique_ptr<std::thread> init_thread;
#endif
#ifdef HAVE_NLOHMANN_JSON_HPP
  void save_tile_index() const;
  void load_tile_index();
#endif

#ifdef TARGET_OS_MACCATALYST
  void add_tile(std::string filename, std::set<std::string> &deleted_tiles);
#else
  void add_tile(std::string filename, map_tile_type &deleted_tiles);
#endif

public:
  ElevationService(const std::string &directory_path,
                   const std::string &index_pathname,
                   const std::string &proj_search_path,
                   long tile_cache_ms);
  ElevationService(const char *directory_path,
                   const char *index_pathname,
                   const char *proj_search_path,
                   long tile_cache_ms)
    : ElevationService(std::string(directory_path),
                       std::string(index_pathname),
                       std::string(proj_search_path), tile_cache_ms) {}
  ~ElevationService();
  /// Constant indicating the coordinate has no elevation data
  static const double no_data;
  std::optional<double> get_elevation(double longitude, double latitude);
  double get_elevation_as_double(double longitude, double latitude) {
    const auto retval = get_elevation(longitude, latitude);
    if (retval.has_value()) {
      return retval.value();
    } else {
      return ElevationTile::no_data;
    }
  }
  bool add_tile(const std::string &pathname);
  bool add_tile(const char *pathname) {
    return add_tile(std::string(pathname));
  }
  bool delete_tile(const std::string &filename);
  bool delete_tile(const char *filename) {
    return delete_tile(std::string(filename));
  }
  void update_tile_cache();
  bool empty() {
    return tiles.empty();
  }

  /**
   * Iterates across the set of points, filling in elevation values where a
   * value is available.
   *
   * \param force replaces all elevation values with those in the dataset, if
   * the dataset has a value, otherwise the original value is left as-is.
   *
   * \param skip_all_if_any_exist if there is already an elevation value for one
   * or more points, no points will be updated even if force has been specified.
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
      if (i->altitude.has_value())
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

        if (!point->altitude.has_value() || force) {
          auto altitude = get_elevation(point->longitude,
                                        point->latitude);
          if (force) {
            if (altitude.has_value())
              point->altitude = altitude;
          } else {
            point->altitude = altitude;
          }
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

    for (auto &i = begin; i != end; ++i)
      fill_elevations(
          i->points.begin(),
          i->points.end(),
          force,
          skip_all_if_any_exist);
  }

};

} // namespace trip
} // namespace fdsd

#endif // HAVE_GDAL

#endif // ELEVATION_TILE_HPP
