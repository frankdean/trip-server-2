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
#include "geo_utils.cpp"

using namespace fdsd::trip;

// Tests the example for anti-meridian cutting from the RFC.
bool test_rfc7946_example()
{
  std::vector<location> test_path;
  location p1(1, 170.0, 45.0);
  location p2(2, -170.0, 45.0);
  test_path.push_back(p1);
  test_path.push_back(p2);
  GeoUtils g;
  g.add_path(test_path.begin(), test_path.end());
  const auto result = g.as_geojson();
  bool retval = result.dump() == "{\"type\":\"MultiLineString\",\"coordinates\":"
    "[[[170.0,45.0],[180.0,45.0]],"
    "[[-180.0,45.0],[-170.0,45.0]]]}";
  if (!retval) {
    std::cout << "test_rfc7946_example() failed\n" << result.dump() << '\n';
  }
  return retval;
}

// Simple test for a LineString
bool test_linestring()
{
  std::vector<location> test_path;
  test_path.push_back(location(1, 10.0, 45.0));
  test_path.push_back(location(1, 20.0, 43.0));
  test_path.push_back(location(1, 25.0, 61.0));
  GeoUtils g;
  g.add_path(test_path.begin(), test_path.end());
  const auto result = g.as_geojson();
  return result.dump() == "{\"type\":\"LineString\",\"coordinates\":[[10.0,45.0],[20.0,43.0],[25.0,61.0]]}";
}

// Simple test for a Point
bool test_point()
{
  std::vector<location> test_path;
  test_path.push_back(location(1, 10.0, 45.0));
  GeoUtils g;
  g.add_path(test_path.begin(), test_path.end());
  const auto result = g.as_geojson();
  return result.dump() == "{\"type\":\"Point\",\"coordinates\":[10.0,45.0]}";
}

bool test_empty_path()
{
  std::vector<location> test_path;
  GeoUtils g;
  g.add_path(test_path.begin(), test_path.end());
  const auto result = g.as_geojson();
  bool retval = result.dump() == "null";
  if (!retval) {
    std::cout << "test_empty_path() failed\n" << result.dump() << '\n';
  }
  return retval;
}

int main(void)
{
  return !(
      test_rfc7946_example() &&
      test_linestring() &&
      test_point() &&
      test_empty_path()
    );
}
