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
#include "elevation_tile.cpp"
#include "geo_utils.hpp"
#include "../trip-server-common/src/file_utils.hpp"
#include <iostream>
#ifdef HAVE_CXX17
#include <filesystem>
#endif
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef HAVE_GDAL

using namespace fdsd::trip;
// using namespace fdsd::utils;

// Don't use a relative directory as the working directory is different when
// being run by `make check` than when run as a standalone executable.

std::string elevation_tile_dir;
std::unique_ptr<ElevationService> service;

location p1(-0.134583, 51.500194);
location p2(-0.491638, 51.493355);
location p3(-1.093140, 51.500194);
location p4(-1.562805, 51.518998);
location p5(-3.1805419921875, 58.6054722343032);
location p6(-1.2944233417511, 60.3088842900007);
location p7(-7.108154296875, 62.1757596207084);
location p8(-2.296142578125, 63.3816786930298);
location no_data_expected_for_point(-5.45608520507813, 56.5950344674725);

std::vector<location> test_points_1 = {
  p1,
  p2,
  p3,
  p4,
  p5,
  p6,
  p7,
  p8,
};

bool test_single_point()
{
  try {
    auto result = service->get_elevation(p1.longitude, p1.latitude);
    const bool retval = result.has_value() && std::lround(result.value()) == 14;
    if (!retval) {
      std::cerr << "test_single_point() failed, expected 14 but was ";
      if (result.has_value()) {
        std::cerr << result.value();
      } else {
        std::cerr << "not found";
      }
      std::cerr << '\n';
    }
    return retval;
  } catch (const fdsd::utils::FileUtils::DirectoryAccessFailedException &e) {
    std::cerr << "Exception in test_single_point(): "
              << e.what() << "\n" << "Does the directory \""
              << elevation_tile_dir << "\" exist and is it readable?\n";
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Exception in test_single_point(): "
              << e.what() << '\n';
    return false;
  }
}

void set_points(
    std::vector<location> &points,
    std::optional<double> value)
{
  for (auto &i : points)
    i.altitude = value;
}

void clear_points(std::vector<location> &points)
{
  set_points(points, std::optional<double>());
}

void list_points(const std::vector<location> &points)
{
  for (const auto &i : points) {
    std::cerr << "point: " << i.longitude << ", " << i.latitude;
    if (i.altitude.has_value())
      std::cerr << ", altitude: " << i.altitude.value();
    std::cerr << '\n';
  }
}

bool test_passing_set_of_points()
{
  clear_points(test_points_1);
  service->fill_elevations(test_points_1.begin(),
                          test_points_1.end());
  auto i = test_points_1.begin();
  bool retval = i->altitude.has_value() && i->altitude.value() == 14 &&
    (*++i).altitude.has_value() && i->altitude.value() == 35 &&
    (*++i).altitude.has_value() && i->altitude.value() == 117;
  if (!retval) {
    std::cerr << "test_passing_set_of_points() failed:\n";
    list_points(test_points_1);
  }
  return retval;
}

bool test_fill_only_empty_values_skip_false()
{
  set_points(test_points_1, std::optional<double>(9999));
  auto i = test_points_1.begin();
  ++i;
  // This is now the second item in the vector
  i->altitude = std::optional<double>();
  // list_points(test_points_1);
  service->fill_elevations(test_points_1.begin(),
                          test_points_1.end());
  i = test_points_1.begin();
  bool retval = i->altitude.has_value() && i->altitude.value() == 9999 &&
    (*++i).altitude.has_value() && i->altitude.value() == 35 &&
    (*++i).altitude.has_value() && i->altitude.value() == 9999;
  if (!retval) {
    std::cerr << "test_fill_only_empty_values_skip_false() failed:\n";
    list_points(test_points_1);
  }
  return retval;
}

bool test_fill_only_empty_values_skip_true()
{
  set_points(test_points_1, std::optional<double>(9999));
  auto i = test_points_1.begin();
  ++i;
  // This will be the second item in the vector
  i->altitude = std::optional<double>();
  // list_points(test_points_1);
  service->fill_elevations(test_points_1.begin(),
                          test_points_1.end(),
                          false,
                          true);
  i = test_points_1.begin();
  bool retval = i->altitude.has_value() && i->altitude.value() == 9999 &&
    !(*++i).altitude.has_value() &&
    (*++i).altitude.has_value() && i->altitude.value() == 9999;
  if (!retval) {
    std::cerr << "test_fill_only_empty_values_skip_false() failed:\n";
    list_points(test_points_1);
  }
  return retval;
}

bool test_fill_all_values_skip_false()
{
  set_points(test_points_1, std::optional<double>(9999));
  auto i = test_points_1.begin();
  ++i;
  // This will be the second item in the vector
  i->altitude = std::optional<double>();
  // list_points(test_points_1);
  service->fill_elevations(test_points_1.begin(),
                          test_points_1.end(),
                          true,
                          false);
  i = test_points_1.begin();
  bool retval = i->altitude.has_value() && i->altitude.value() == 14 &&
    (*++i).altitude.has_value() && i->altitude.value() == 35 &&
    (*++i).altitude.has_value() && i->altitude.value() == 117;
  if (!retval) {
    std::cerr << "test_fill_only_empty_values_skip_false() failed:\n";
    list_points(test_points_1);
  }
  return retval;
}

bool test_fill_all_values_skip_true()
{
  set_points(test_points_1, std::optional<double>(9999));
  auto i = test_points_1.begin();
  ++i;
  // This will be the second item in the vector
  i->altitude = std::optional<double>();
  // list_points(test_points_1);
  service->fill_elevations(test_points_1.begin(),
                          test_points_1.end(),
                          true,
                          true);
  i = test_points_1.begin();
  bool retval = i->altitude.has_value() && i->altitude.value() == 9999 &&
    !(*++i).altitude.has_value() &&
    (*++i).altitude.has_value() && i->altitude.value() == 9999;
  if (!retval) {
    std::cerr << "test_fill_only_empty_values_skip_false() failed:\n";
    list_points(test_points_1);
  }
  return retval;
}

bool test_fill_all_values_skip_true_all_empty()
{
  clear_points(test_points_1);
  auto i = test_points_1.begin();
  ++i;
  // This will be the second item in the vector
  i->altitude = std::optional<double>();
  // list_points(test_points_1);
  service->fill_elevations(test_points_1.begin(),
                          test_points_1.end(),
                          true,
                          true);
  i = test_points_1.begin();
  bool retval = i->altitude.has_value() && i->altitude.value() == 14 &&
    (*++i).altitude.has_value() && i->altitude.value() == 35 &&
    (*++i).altitude.has_value() && i->altitude.value() == 117;
  if (!retval) {
    std::cerr << "test_fill_only_empty_values_skip_false() failed:\n";
    list_points(test_points_1);
  }
  return retval;
}

#endif // HAVE_GDAL

int main(void)
{
#ifdef HAVE_CXX17
  std::cout << "Current working directory: "
            << std::filesystem::current_path()
            << "\n";
#endif
#ifndef HAVE_GDAL
  std::cerr << "GDAL disabled.  Skipping elevation tile tests\n";
  return 0;
#else
  if (const char* dir = std::getenv("TRIP_ELEVATION_TILE_PATH"))
    elevation_tile_dir = std::string(dir);
  if (elevation_tile_dir.empty()) {
    std::cerr << "TRIP_ELEVATION_TILE_PATH not set.  Skipping elevation tile tests\n";
    return 0;
  }
  const std::string index_tile_pathname = elevation_tile_dir +
    FileUtils::path_separator + ".tile-index.json";
  service = std::unique_ptr<ElevationService>(
      new ElevationService(elevation_tile_dir,
                           index_tile_pathname,
                           std::string(),
                           0));
  try {
    return !(
        test_single_point() &&
        test_passing_set_of_points() &&
        test_fill_only_empty_values_skip_false() &&
        test_fill_only_empty_values_skip_true() &&
        test_fill_all_values_skip_false() &&
        test_fill_all_values_skip_true() &&
        test_fill_all_values_skip_true_all_empty()
      );
  } catch (const fdsd::utils::FileUtils::DirectoryAccessFailedException &e) {
    std::cerr
      << "Tests failed with: " << e.what() << '\n'
      << "Most probably caused by the elevation tiles directory "
      "not existing\n";
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Tests failed with: " << e.what() << '\n';
    return 1;
  }
#endif
}
