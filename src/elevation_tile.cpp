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
#if ! BUILD_FOR_IOS
#include "../config.h"
#endif

#ifdef HAVE_GDAL
#include "elevation_tile.hpp"
#include <filesystem>
#include <iostream>
#ifdef HAVE_BOOST
#include <boost/locale.hpp>
#endif
#include <gdal_priv.h>
#ifdef HAVE_PROJ
#include <proj.h>
#endif
#if BUILD_FOR_IOS
#include "file_utils.hpp"
#else
#include "../trip-server-common/src/file_utils.hpp"
#endif
#include <string>
#if !BUILD_FOR_IOS
#define USE_SYSLOG
#include <syslog.h>
#endif
#include <fstream>
#include <sstream>

#ifdef HAVE_BOOST
using namespace boost::locale;
#endif
using namespace fdsd::trip;
using namespace fdsd::utils;
#ifdef HAVE_NLOHMANN_JSON_HPP
using json = nlohmann::basic_json<nlohmann::ordered_map>;
#endif

bool ElevationTile::drivers_registered = false;
const double ElevationTile::no_data = -32768;
const double ElevationService::no_data = ElevationTile::no_data;

namespace fdsd::trip {

#ifdef HAVE_NLOHMANN_JSON_HPP
void to_json(json& j, const ElevationTile& t)
// void to_json(nlohmann::basic_json<nlohmann::ordered_map>& j, const ElevationTile& t)
{
  ElevationTile::to_json(j, t);
}

void from_json(const json& j, ElevationTile& t)
{
  ElevationTile::from_json(j, t);
}
#endif

} // namespace fdsd::trip

std::mutex ElevationTile::dataset_mutex;

#ifdef HAVE_NLOHMANN_JSON_HPP
ElevationTile::ElevationTile(json j)
  : dataset(), coordinate_transform(), time()
{
  ElevationTile::from_json(j, *this);
  // time() defaults to zero (beginning of epoch)
}
#endif

ElevationTile::ElevationTile(std::string directory_path, std::string filename)
  : filename(filename), top(), right(), bottom(),
    left(), xskew(), yskew(), pixel_width(), pixel_height(), dataset(),
    coordinate_transform(), time()
{
  // std::cout << "Opening dataset for elevation tile at: \"" << filename << "\"\n";
  open(directory_path);
  double geotransform[6];
  if (dataset->GetGeoTransform(geotransform) != CE_None)
    throw dataset_exception(CPLGetLastErrorMsg());

  // std::cout << "Origin = (" << geotransform[0] << ','
  //           << geotransform[3] << ")\n";
  // std::cout << "Pixel size = (" << geotransform[1] << ','
  //           << geotransform[5] << ")\n";

  left = geotransform[0];
  pixel_width = geotransform[1];
  xskew = geotransform[2];
  top = geotransform[3];
  yskew = geotransform[4];
  pixel_height = geotransform[5];
  right = left + dataset->GetRasterXSize() * pixel_width;
  bottom = top + dataset->GetRasterYSize() * pixel_height;
}

void ElevationTile::open(std::string directory_path)
{
  auto gdal_path = ElevationTile::build_gdal_path_name(directory_path, filename);
  // std::cout << "Opening tile: " << filename << '\n';
  if (!ElevationTile::drivers_registered) {
    // std::cout << "Registering GDAL drivers\n";
    GDALAllRegister();
    ElevationTile::drivers_registered = true;
  }
  std::lock_guard<std::mutex> lock(dataset_mutex);
  if ((dataset = (GDALDataset *) GDALOpen(gdal_path.c_str(), GA_ReadOnly)) == nullptr) {
    // auto error_no = CPLGetLastErrorNo();
    // std::cout << "GDL error number: " << error_no << '\n';
    throw dataset_exception(CPLGetLastErrorMsg());
  }

  // std::cout << "Dataset successfully opened\n";
  // std::cout << "Dataset driver: "
  //           << dataset->GetDriver()->GetDescription() << '/'
  //           << dataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME)
  //           << '\n';
  // std::cout << "Dataset size: "
  //           << dataset->GetRasterXSize() << 'x'
  //           << dataset->GetRasterYSize() << 'x'
  //           << dataset->GetRasterCount() << '\n';
  // std::cout << "Dataset has " << dataset->GetRasterCount() << " band(s)\n";
  if (dataset->GetProjectionRef() == nullptr)
    throw dataset_exception(CPLGetLastErrorMsg());

  const std::string raster_projection = dataset->GetProjectionRef();
  // std::cout << "Projection is \"" << raster_projection << "\"\n";
  OGRSpatialReference target_srs(raster_projection.c_str());
  OGRSpatialReference source_srs;
  source_srs.importFromEPSG(4326); // EPSG for WGS84
  // source_srs.SetWellKnownGeogCS("WGS84");

  if ((coordinate_transform = OGRCreateCoordinateTransformation(
           &source_srs,
           &target_srs)) == nullptr)
    throw dataset_exception(CPLGetLastErrorMsg());

  if ((band = dataset->GetRasterBand(1)) == nullptr)
    throw dataset_exception(CPLGetLastErrorMsg());
  // std::cout << "Updating time stamp for " << *path << '\n';
  time = std::chrono::system_clock::now();
}

void ElevationTile::close()
{
  // std::cout << "Close called for: " << filename << '\n';
  // We get called for every tile, including already closed ones,
  // so avoid unnecessary lock etc.
  if (dataset == nullptr && coordinate_transform == nullptr)
    return;
  std::lock_guard<std::mutex> lock(dataset_mutex);
  if (dataset != nullptr) {
    // std::cout << "Closing data set for tile: " << filename << '\n';
    GDALClose(dataset);
    dataset = nullptr;
  }
  if (coordinate_transform != nullptr) {
    // https://gdal.org/en/release-3.10/doxygen/classOGRCoordinateTransformation.html#a547d9277bf137105815ffb65a604fbe4
    // OGRCoordinateTransformation::DestroyCT(coordinate_transform);
    delete coordinate_transform;
    coordinate_transform = nullptr;
  }
}

std::optional<double> ElevationTile::get_elevation(std::string directory_path,
                                                   double longitude,
                                                   double latitude)
{
  // std::cout << "ElevationTile::get_elevation\n";
  if (dataset == nullptr) {
    open(directory_path);
  }
  std::lock_guard<std::mutex> lock(dataset_mutex);
  time = std::chrono::system_clock::now();
  double x = longitude;
  double y = latitude;
  if (!coordinate_transform->Transform(1, &x, &y)) {
    std::cerr << "Transformation of lon: " << longitude << " lat: " << latitude
              << " failed\n";
    #ifdef USE_SYSLOG
    syslog(LOG_ERR, "Transformation of lon: %.6f lat: %.6f failed", longitude, latitude);
    #else
    std::cerr << "Transformation of lon: " << longitude << ", lat: " << latitude << " failed\n";
    #endif
    if (CPLGetLastErrorNo() != CPLE_None) {
      throw dataset_exception(CPLGetLastErrorMsg());
    } else {
      throw dataset_exception("Transformation failed");
    }
  }

  const double x_offset = (x - left - y * xskew) / pixel_width;
  const double y_offset = (y - top - x * yskew) / pixel_height;
  // std::cout << "Converted x: " << longitude << ", y: " << latitude
  //           << ", to x: " << x_offset << ", y: " << y_offset << '\n';
  float *scanline;
  const int size = 1;
  // std::cout << "Band x size: " << size << '\n';
  scanline = (float *) CPLMalloc(sizeof(float) * size);
  const CPLErr err = band->RasterIO(GF_Read,
                                    x_offset,
                                    y_offset,
                                    size, // nXSize
                                    1, // nYSize
                                    scanline, //pData
                                    size, // nBufXSize
                                    1, // nBufYSize
                                    GDT_Float32, // eBufType
                                    0, // nPixelSpace (default)
                                    0, // nLineSpace (default)
                                    nullptr // psExtraArg (default)
    );
  if (err == CE_Failure)
    throw dataset_exception(CPLGetLastErrorMsg());

  const double elevation = *scanline;
  const bool has_data = elevation != no_data;
  // if (!has_data) {
  //   std::cout << "No data\n";
  // }
  CPLFree(scanline);
  std::optional<double> retval;
  if (has_data)
    retval = elevation;
  return retval;
}

/// Returns all characters from the final name up to but not including the final period/dot.
std::string ElevationTile::strip_file_extension(std::string filename)
{
  auto name = filename;
  auto p = name.find_last_of(".");
  if (p != std::string::npos && p > 1) {
    name = name.substr(0, p);
    // std:: cout << "Name: \"" << name << "\"\n";
  }
  return name;
}

std::string ElevationTile::build_gdal_path_name(
    std::string directory_path,
    std::string filename)
{
  // std::cout << "building GDAL pathname from directory \"" << directory_path << "\" and filename: \"" << filename << "\"\n";

  auto geo_tiff_file_type = get_geo_tiff_tile_type(filename);
  auto extension = FileUtils::get_extension(filename);

  // Note: Datasets can be opened from compressed tar or zip files
  // https://gdal.org/user/virtual_file_systems.html
  // The filename simply needs to be specified in the form of
  // /vsitar//path/to.tar/path/within/tar.  Omit the double-slash to make the
  // path relative to the current working directory.

  std::string retval;
  switch (geo_tiff_file_type) {
    case zip: {
      auto name = strip_file_extension(filename);
      retval = "/vsizip/" + directory_path + FileUtils::path_separator + filename +
        FileUtils::path_separator + name + ".tif";
      break;
    }
    case tar:
    case tgz: {
      auto name = strip_file_extension(filename);
      retval = "/vsitar/" + directory_path + FileUtils::path_separator + filename +
        FileUtils::path_separator + name + ".tif";
      break;
    }
    case tiff:
      retval = directory_path + FileUtils::path_separator + filename;
      break;
    default:
      break;
  }
  return retval;
}

ElevationTile::geo_tiff_file_types
    ElevationTile::get_geo_tiff_tile_type(const std::string filename)
{
  auto extension = FileUtils::get_extension(filename);
  if (extension == "tif")
    return fdsd::trip::ElevationTile::tiff;
  if (extension == "tar")
    return fdsd::trip::ElevationTile::tar;
  if (extension == "tgz")
    return fdsd::trip::ElevationTile::tgz;
  if (extension == "zip")
    return fdsd::trip::ElevationTile::zip;
  return fdsd::trip::ElevationTile::unknown;
}


#ifdef HAVE_NLOHMANN_JSON_HPP
void ElevationTile::to_json(json& j, const ElevationTile& t)
{
  j = json{
    {"filename", t.filename},
    {"top", t.top},
    {"right", t.right},
    {"bottom", t.bottom},
    {"left", t.left},
    {"xskew", t.xskew},
    {"yskew", t.yskew},
    {"pixel_width", t.pixel_width},
    {"pixel_height", t.pixel_height}
  };
}

void ElevationTile::from_json(const json& j, ElevationTile& t)
{
  std::string filename;
  j.at("filename").get_to(filename);
  t.filename = filename;
  j.at("top").get_to(t.top);
  j.at("right").get_to(t.right);
  j.at("bottom").get_to(t.bottom);
  j.at("left").get_to(t.left);
  j.at("xskew").get_to(t.xskew);
  j.at("yskew").get_to(t.yskew);
  j.at("pixel_width").get_to(t.pixel_width);
  j.at("pixel_height").get_to(t.pixel_height);
}

#endif // HAVE_NLOHMANN_JSON_HPP

/**
 * \param directory_path path to the directory containing the elevation tile tif
 * files.
 * \param index_pathname path to the name of the file to use to store the JSON
 * directory index.
 * \param proj_search_path if PROJ fails to find proj.db, set this to it's path.
 * It should be part of the distribution.
 * \param tile_cache_ms the period of time to cache elevation tiles for.
 * Zero (or less) disables caching.
 */
ElevationService::ElevationService(const std::string &directory_path,
                                   const std::string &index_pathname,
                                   const std::string &proj_search_path,
                                   long tile_cache_ms)
  : directory_path(directory_path),
    index_pathname(index_pathname),
    tile_cache_ms(tile_cache_ms),
    tiles(),
    initialized(false),
    initialization_error()
#ifdef HAVE_THREAD
  , init_thread(new std::thread(&ElevationService::init, this))
#endif
{
  #if BUILD_FOR_IOS
  try {
    #endif
    #ifdef USE_SYSLOG
    syslog(LOG_DEBUG, "Initializing elevation service");
    #else
    std::cerr << "Initializing elevation service\n";
    #endif
    // std::cout << "Tiles will be caches for " << tile_cache_ms << " milliseconds\n";
    #ifdef HAVE_PROJ
    if (!proj_search_path.empty()) {
      std::vector<const char*> vc(1);
      vc[0] = proj_search_path.c_str();
      proj_context_set_search_paths(nullptr, 1, vc.data());
      // std::cout << "Set proj.db search path to " << proj_search_path << '\n';
    }
    #else
    (void)proj_search_path; // unused
    #endif
    #ifndef HAVE_THREAD
    init();
    #endif
    #if BUILD_FOR_IOS
  } catch (const std::exception &e) {
    // Avoid exceptions when running on IOS as it crashes the app
    std::cerr << "Exception initializing elevation tiles: " << e.what() << '\n';
  }
  #endif
}

void ElevationService::init()
{
  try {
    auto start = std::chrono::system_clock::now();
    #ifdef HAVE_NLOHMANN_JSON_HPP
    load_tile_index();
    // std::cout << "Loaded " << tiles.size() << " tiles from index\n";
    #endif

    #ifdef TARGET_OS_MACCATALYST
    std::set<std::string> deleted_tiles;
    for (const auto &t : tiles) {
      deleted_tiles.insert(t.filename);
    }
    #else
    // Create copy from which to remove tiles as they are found.  Remaining
    // tiles must have been deleted.
    map_tile_type deleted_tiles = tiles;
    #endif

    auto dir_list = FileUtils::get_directory(directory_path);
    for (const auto &entry : dir_list) {
      auto extension = FileUtils::get_extension(entry.name);
      if (entry.type != FileUtils::regular_file)
        continue;

      auto geo_tiff_file_type =
        ElevationTile::get_geo_tiff_tile_type(entry.name);
      if (geo_tiff_file_type == ElevationTile::unknown)
        continue;

      try {
        add_tile(entry.name, deleted_tiles);
      } catch (const ElevationTile::dataset_exception &e) {
        std::cerr << "Error adding file: \"" << entry.name
                  << "\" to elevation tile list: " << e.what() << '\n';

        #ifdef USE_SYSLOG
        syslog(LOG_ERR, "Error adding file: \"%s\" to elevation tile list: %s",
               entry.name.c_str(),
               e.what());
        #else
        std::cerr << "Error adding file: \"" << entry.name
                  << "\" to elevation tile list: " << e.what() << '\n';
        #endif
      }
    } // for

    #ifdef TARGET_OS_MACCATALYST
    for (auto t = tiles.begin(); t != tiles.end();) {
      if (auto it = deleted_tiles.find(t->filename) != deleted_tiles.end()) {
        tiles.erase(t);
      } else {
        t++;
      }
    }
    #else
    for (auto& m : deleted_tiles)
      tiles.erase(m.first);
    #endif

    auto finish = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> diff = finish - start;
    std::stringstream msg;
    // Message displayed after loading elevation tiles
    #ifdef HAVE_BOOST_LOCALE
    msg << format(translate("Loaded {1} elevation tiles in {2} ms"))
      % tiles.size() % diff.count();
    #else
    msg << "Loaded " << tiles.size() << " elevation tiles in "
        << diff.count() << " ms";
    #endif
    #ifdef USE_SYSLOG
    syslog(LOG_INFO, "%s", msg.str().c_str());
    #else
    std::cerr << msg.str() << '\n';
    #endif
  } catch (const std::exception &e) {
    #ifdef USE_SYSLOG
    syslog(LOG_ERR, "Exception initializing elevation tiles: %s", e.what());
    #else
    std::cerr << "Exception initializing elevation tiles:" << e.what() << '\n';
    #endif
    initialization_error = std::current_exception();
  }
  initialized = true;
  #ifdef HAVE_NLOHMANN_JSON_HPP
  save_tile_index();
  #endif
}

ElevationService::~ElevationService()
{
  #ifdef HAVE_THREAD
  if (init_thread) {
    init_thread->join();
  }
  #endif
}

/// @param filename - just the filename without the full path
/// @param deleted_tiles - a map containing tiles that are to be ignored, normally a
///  list of existing tiles, but can be empty when the file is known not to exist.
void ElevationService::add_tile(std::string filename,
  #ifdef TARGET_OS_MACCATALYST
                                std::set<std::string> &deleted_tiles) {
  #else
                                map_tile_type &deleted_tiles) {
  #endif
  // std::cout << "Checking \"" << filename << "\"\n";
  auto gdal_ref = build_gdal_path_name(filename);
  if (!gdal_ref.empty()) {
    if (auto search = deleted_tiles.find(filename); search != deleted_tiles.end()) {
      // std::cout << "Skipping entry name " << filename << '\n';
      deleted_tiles.erase(search);
    } else {
      // std::cout << "Entry name \"" << filename << "\" does not exist in the index\n";
      ElevationTile tile(directory_path, filename);
      // If caching is enabled, close the tile after parsing it
      if (tile_cache_ms > 0)
        tile.close();

      // std::cout << "Adding\"" << filename << "\"\n";
      #ifdef TARGET_OS_MACCATALYST
      tiles.push_back(tile);
      #else
      tiles.emplace(tile.filename, tile);
      #endif
    }
  }
}

/// @param filename - just the filename without the full path
bool ElevationService::add_tile(const std::string &filename) {
  try {
    #ifdef TARGET_OS_MACCATALYST
    std::set<std::string> deleted_tiles;
    #else
    // Create an empty map of the tile paths
    map_tile_type deleted_tiles;
    #endif
    add_tile(filename, deleted_tiles);
    #ifdef HAVE_NLOHMANN_JSON_HPP
    save_tile_index();
    #endif
    return true;
  } catch (const ElevationTile::dataset_exception &e) {
    // Avoid exceptions when running on IOS as it crashes the app
    std::cerr << "Exception getting elevation data: " << e.what() << '\n';
    #if BUILD_FOR_IOS
  } catch (const std::exception &e) {
    // Avoid exceptions when running on IOS as it crashes the app
    std::cerr << "Exception getting elevation data: " << e.what() << '\n';
    #endif
  }
  return false;
}

  bool ElevationService::delete_tile(const std::string &filename)
  {
    #if BUILD_FOR_IOS
    try {
      #endif
      #ifdef TARGET_OS_MACCATALYST
      auto pos = std::find_if(tiles.begin(), tiles.end(),
                              [=] (const auto &t) {
        return filename == t.filename;
      });
      #else
      auto pos = tiles.find(filename);
      #endif
      // Delete the specified file without checking if it's in the index due to
      // the potential presence of other files, e.g. '.properties' files that
      // GDAL creates
      std::string pathname = directory_path + FileUtils::path_separator + filename;
      // std::cout << "Deleting \"" << pathname << "\"\n";
      auto retval = std::filesystem::remove(pathname);
      if (pos != tiles.end())
        tiles.erase(pos);
      #ifdef HAVE_NLOHMANN_JSON_HPP
        save_tile_index();
        #endif
      return retval;
      #if BUILD_FOR_IOS
    } catch (const std::exception &e) {
      std::cerr << "Failed to delete \"" << filename << '"' << e.what() << '\n';
    }
    #endif
    return false;
  }

std::string ElevationService::build_gdal_path_name(
    std::string filename) const
{
  auto extension = FileUtils::get_extension(filename);
  return ElevationTile::build_gdal_path_name(directory_path, filename);
}

void ElevationService::update_tile_cache()
{
  #if BUILD_FOR_IOS
  try {
  #endif
    if (tile_cache_ms <= 0)
      return;
    for (auto &t : tiles) {
      auto now = std::chrono::system_clock::now();
      #ifdef TARGET_OS_MACCATALYST
      const std::chrono::duration<double, std::milli> diff = now - t.time ;
      if (diff.count() >= tile_cache_ms)
        t.close();
      #else // TARGET_OS_MACCATALYST
      const std::chrono::duration<double, std::milli> diff = now - t.second.time ;
      if (diff.count() >= tile_cache_ms)
        t.second.close();
      #endif // TARGET_OS_MACCATALYST
    }
    #if BUILD_FOR_IOS
  } catch (const std::exception &e) {
    std::cerr << "Error whilst updating tile cache: " << e.what() << '\n';
  }
  #endif
}

/**
 * Returns a std::pair, the first element true if there is an elevation value
 * and the second element containing the elevation value for the specified
 * coordinates.
 */
std::optional<double>
    ElevationService::get_elevation(double longitude, double latitude)
{
  auto retval = std::optional<double>();
  #if BUILD_FOR_IOS
  try {
    #endif
    #ifdef HAVE_THREAD
    while (!initialized) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    #endif
    if (initialization_error) {
      std::cerr << "FAILURE INIT\n";
      #ifdef USE_SYSLOG
      syslog(LOG_ERR, "Failed to initialize elevation service");
      #else
      std::cerr << "Failed to initialize elevation service\n";
      #endif
      std::rethrow_exception(initialization_error);
    }
    try {
      #ifdef TARGET_OS_MACCATALYST
      auto pos = std::find_if(tiles.begin(), tiles.end(),
                              [=] (const auto &t) {
                                return longitude >= t.left &&
                                  longitude <= t.right &&
                                  latitude >= t.bottom &&
                                  latitude <= t.top;
                              });
      #else
      auto pos = std::find_if(tiles.begin(), tiles.end(),
                              [=] (const auto &t) {
                                return longitude >= t.second.left &&
                                  longitude <= t.second.right &&
                                  latitude >= t.second.bottom &&
                                  latitude <= t.second.top;
                              });
      #endif
      if (pos != tiles.end()) {
        #ifdef TARGET_OS_MACCATALYST
        retval = (*pos).get_elevation(directory_path, longitude, latitude);
        #else
        retval = (*pos).second.get_elevation(directory_path, longitude, latitude);
        #endif
      }
    } catch (const ElevationTile::dataset_exception &e) {
      std::cerr << e.what() << '\n';

      #ifdef USE_SYSLOG
      syslog(LOG_ERR, "Error getting elevation for lon: %.6f lat: %.6f: %s",
             longitude,
             latitude,
             e.what());
      #else
      std::cerr << "Error getting elevation for lon: " << longitude
                << ", lat: " << latitude << ": " << e.what() << '\n';
      #endif
    }
    update_tile_cache();
    #if BUILD_FOR_IOS
  } catch (const std::exception &e) {
    // Avoid exceptions when running on IOS as it crashes the app
    std::cerr << "Exception getting elevation data: " << e.what() << '\n';
  }
  #endif
  return retval;
}

#ifdef HAVE_NLOHMANN_JSON_HPP

void ElevationService::save_tile_index() const
{
  // std::cout << "Saving " << tiles.size() << " tiles\n";
  json j;
  for (const auto &t : tiles) {
    #ifdef TARGET_OS_MACCATALYST
    j.push_back(t);
    #else
    j.push_back(t.second);
    #endif
  }
  // std::cout << j.dump();
  std::ofstream ofs(index_pathname);
  ofs << j << std::endl;
}

void ElevationService::load_tile_index()
{
  try {
    std::ifstream ifs(index_pathname);
    json j;
    ifs >> j;
    try {
      for (auto& j_tile : j) {
        ElevationTile tile(j_tile);
        #ifdef TARGET_OS_MACCATALYST
        tiles.push_back(tile);
        #else
        tiles.emplace(tile.filename,  tile);
        #endif
      }
    } catch (const nlohmann::detail::parse_error &e) {
      std::cerr << "Json parsing error creating elevation tile: " << e.what() << '\n';
    } catch (const nlohmann::detail::exception &e) {
      std::cerr << "Json exception creating elevation tile: " << e.what() << '\n';
    }
  } catch (const nlohmann::detail::parse_error &e) {
    std::cerr << "Error: \"" << index_pathname << "\" does not exist: " << e.what() << '\n';
  } catch (const std::exception &e) {
    std::cerr << "Exception creating elevation tiles: " << e.what() << '\n';
  }
}

#endif // HAVE_NLOHMANN_JSON_HPP

#endif // HAVE_GDAL
