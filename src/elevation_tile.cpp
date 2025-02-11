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
#ifdef HAVE_GDAL
#include "elevation_tile.hpp"
#include <iostream>
#include <boost/locale.hpp>
#include <gdal_priv.h>
#include "../trip-server-common/src/file_utils.hpp"
#include <syslog.h>
#include <sstream>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::utils;

bool ElevationTile::drivers_registered = false;
const int ElevationTile::no_data = -32768;

ElevationTile::ElevationTile(std::string path, int cache_ms)
  : path(path), cache_ms(cache_ms), left(), pixel_width(), xskew(), top(),
    yskew(), pixel_height(), right(), bottom(), dataset(), band(),
    coordinate_transform(), time(), dataset_mutex()
{
  // std::cout << "Opening dataset for elevation tile at: \"" << path << "\"\n";
  if (!ElevationTile::drivers_registered) {
    GDALAllRegister();
    ElevationTile::drivers_registered = true;
  }
  open();
  double geotransform[6];
  if (dataset->GetGeoTransform(geotransform) != CE_None)
    throw dataset_exception("Data set does specify a projection");

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

ElevationTile::~ElevationTile()
{
  close();
}

void ElevationTile::open()
{
  // std::cout << "Opening tile: " << path << '\n';

  std::lock_guard<std::mutex> lock(dataset_mutex);
  if ((dataset = (GDALDataset *) GDALOpen(path.c_str(), GA_ReadOnly)) == NULL)
      throw dataset_exception("Failure opening dataset");

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
  if (dataset->GetProjectionRef() == NULL)
    throw dataset_exception("Unable to get the projection definition");

  const std::string raster_projection = dataset->GetProjectionRef();
  // std::cout << "Projection is \"" << raster_projection << "\"\n";
  OGRSpatialReference target_srs(raster_projection.c_str());
  OGRSpatialReference source_srs;
  source_srs.importFromEPSG(4326); // EPSG for WGS84
  // source_srs.SetWellKnownGeogCS("WGS84");

  if ((coordinate_transform = OGRCreateCoordinateTransformation(
      &source_srs,
      &target_srs)) == NULL)
    throw dataset_exception("Unable to create transformation object");

  if ((band = dataset->GetRasterBand(1)) == NULL)
    throw dataset_exception("Unable to read raster band");
  time = std::chrono::system_clock::now();
}

void ElevationTile::close()
{
  std::lock_guard<std::mutex> lock(dataset_mutex);
  // std::cout << "Closing tile: " << path << '\n';
  if (dataset != NULL) {
    GDALClose(dataset);
    dataset = NULL;
  }
  if (coordinate_transform != NULL) {
    OCTDestroyCoordinateTransformation(coordinate_transform);
    coordinate_transform = NULL;
  }
}

std::optional<double>
    ElevationTile::get_elevation(double longitude, double latitude)
{
  if (dataset == NULL)
    open();
  std::lock_guard<std::mutex> lock(dataset_mutex);
  double x = longitude;
  double y = latitude;
  if (!coordinate_transform->Transform(1, &x, &y)) {
    std::cerr << "Transformation of lon: " << longitude << " lat: " << latitude
              << " failed\n";
    syslog(LOG_ERR, "Transformation of lon: %.6f lat: %.6f failed", longitude, latitude);
    throw dataset_exception("Transformation failed");
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
                                    NULL // psExtraArg (default)
    );
  if (err == CE_Failure)
    throw dataset_exception("Unable to read elevation data from raster tile");

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

/**
 * \param directory_path path to the directory containing the elevation tile tif
 * files.  \param tile_cache_ms the period of time to cache elevation tiles for.
 * Zero (or less) disables caching.
 */
ElevationService::ElevationService(std::string directory_path, long tile_cache_ms)
  : directory_path(directory_path),
    tile_cache_ms(tile_cache_ms),
    tiles(),
    initialized(false),
    initialization_error(),
    init_thread(new std::thread(&ElevationService::init, this))
{
  syslog(LOG_DEBUG, "Initializing elevation service\n");
}

void ElevationService::init()
{
  try {
    auto start = std::chrono::system_clock::now();
    auto dir_list = FileUtils::get_directory(directory_path);
    for (const auto &entry : dir_list) {
      // std::cout << "Checking \"" << entry.name << "\"\n";
      try {
        auto extension = FileUtils::get_extension(entry.name);
        if (entry.type != FileUtils::regular_file ||
            (extension != "tif" && extension != "zip" && extension != "tgz"))
          continue;

        // Note: Datasets can be opened from compressed tar or zip files
        // https://gdal.org/user/virtual_file_systems.html
        // The filename simply needs to be specified in the form of
        // /vsitar//path/to.tar/path/within/tar.  Omit the double-slash to make the
        // path relative to the current working directory.

        std::unique_ptr<ElevationTile> tile;
        if (extension == "zip") {
          auto name = entry.name;
          auto p = name.find_last_of(".");
          if (p != std::string::npos && p > 1) {
            name = name.substr(0, p);
            // std:: cout << "Name: \"" << name << "\"\n";
          }
          std::unique_ptr<ElevationTile> zipTile(
              new ElevationTile(
                  "/vsizip/" + directory_path + FileUtils::path_separator + entry.name +
                  FileUtils::path_separator + name + ".tif")
            );
          tile = std::move(zipTile);
        } else if (extension == "tgz") {
          auto name = entry.name;
          auto p = name.find_last_of(".");
          if (p != std::string::npos && p > 1) {
            name = name.substr(0, p);
          }
          std::unique_ptr<ElevationTile> gzipTile(
              new ElevationTile(
                  "/vsitar/" + directory_path + FileUtils::path_separator + entry.name +
                  FileUtils::path_separator + name)
            );
          tile = std::move(gzipTile);
        } else if (extension == "tif") {
          std::unique_ptr<ElevationTile> tifTile(
              new ElevationTile(
                  directory_path + FileUtils::path_separator + entry.name)
            );
          tile = std::move(tifTile);
        }

        if (tile != nullptr) {
          // If caching is enabled, close the tile after parsing it
          if (tile_cache_ms > 0)
            tile->close();

          tiles.push_back(std::move(tile));

          // std::cout << "Added \"" << entry.name << "\" to set of elevation tiles\n";
        }
      } catch (const ElevationTile::dataset_exception &e) {
        std::cerr << "Error adding file: \"" << entry.name << "\" to elevation tile list: "
                  << e.what() << '\n';
        syslog(LOG_ERR, "Error adding file: \"%s\" to elevation tile list: %s",
               entry.name.c_str(),
               e.what());
      }
    }
    auto finish = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> diff = finish - start;
    std::stringstream msg;
    // Message displayed after loading elevation tiles
    msg << format(translate("Loaded {1} elevation tiles in {2} ms"))
      % tiles.size() % diff.count();
    syslog(LOG_INFO, "%s", msg.str().c_str());
  } catch (const std::exception &e) {
    syslog(LOG_ERR, "Exception initializing elevation tiles: %s",
           e.what());
    initialization_error = std::current_exception();
  }
  initialized = true;
}

ElevationService::~ElevationService()
{
  syslog(LOG_DEBUG, "Closing elevation service\n");
  if (init_thread) {
    init_thread->join();
  }
}

void ElevationService::update_tile_cache()
{
  if (tile_cache_ms <= 0)
    return;
  for (const auto &t : tiles) {
    auto now = std::chrono::system_clock::now();
    const std::chrono::duration<double, std::milli> diff = now - t->time ;
    if (diff.count() >= tile_cache_ms)
      t->close();
  }
}

/**
 * Returns a std::pair, the first element true if there is an elevation value
 * and the second element containing the elevation value for the specified
 * coordinates.
 */
std::optional<double>
    ElevationService::get_elevation(double longitude, double latitude)
{
  while (!initialized) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  if (initialization_error) {
    std::cerr << "FAILURE INIT\n";
    syslog(LOG_ERR, "Failed to initialize elevation service");
    std::rethrow_exception(initialization_error);
  }
    // throw std::runtime_error("Failed to initialize elevation tiles service");
  auto retval = std::optional<double>();
  try {
    auto pos = std::find_if(tiles.begin(), tiles.end(),
                            [=] (std::unique_ptr<ElevationTile> &t) {
                              return longitude >= t->left && longitude <= t->right &&
                                latitude >= t->bottom && latitude <= t->top;
                            });
    if (pos != tiles.end()) {
      retval = (*pos)->get_elevation(longitude, latitude);
    }
  } catch (const ElevationTile::dataset_exception &e) {
    std::cerr << e.what() << '\n';
    syslog(LOG_ERR, "Error getting elevation for lon: %.6f lat: %.6f: %s",
           longitude,
           latitude,
           e.what());
  }
  update_tile_cache();
  return retval;
}

#endif // HAVE_GDAL
