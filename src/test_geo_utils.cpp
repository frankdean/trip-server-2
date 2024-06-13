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
#include "geo_utils.cpp"
#include <memory>
#include <string>
#include <vector>

using namespace fdsd::trip;

location p1(-0.134583, 51.500194, std::optional<double>(100));
location p2(-0.491638, 51.493355, std::optional<double>(110));
location p3(-1.093140, 51.500194, std::optional<double>(30));
location p4(-1.562805, 51.518998, std::optional<double>(200));
location p5(-3.1805419921875, 58.6054722343032, std::optional<double>(200));
location p6(-1.2944233417511, 60.3088842900007, std::optional<double>(30));
location p7(-7.108154296875, 62.1757596207084, std::optional<double>(40));
location p8(-2.296142578125, 63.3816786930298, std::optional<double>(30));

location p10(2.308502, 48.858955, std::optional<double>(42));
location p11(2.325325, 48.885263, std::optional<double>(68));
location p12(2.365150, 48.872732, std::optional<double>(42));
location p13(2.363091, 48.855793, std::optional<double>(41));
location p14(2.314854, 48.858052, std::optional<double>(38));

location p20(-8.4844798208508632, -6.602169639459575);
location p21(-4.6028008208115141, 12.543667966533093);
location p22(15.3182687642961, 16.653402118751146);
location p23(11.29011131142507, -0.5379650767794999);
location p24(-0.94083950001967287, -1.7827521027885638);

location p30(27.505040001324652, 23.940840900885192);
location p31(29.557840949939404, 18.738117112941978);
location p32(21.802815144061455, 19.169560130569266);
location p33(26.820773018453064, 21.150000209898565);
location p34(14.161833835328773, 24.771984788473262);

location p40(-48.511082038978074, -22.531824919650418);
location p41(-22.058775552428152, -11.59605781852288);
location p42(-24.413580284168496, -22.749158714957133);
location p43(-27.396332944372936, -17.593834004707105);
location p44(-27.239345962256916, -21.074306115699372);
location p45(-36.109110451812228, -19.158085277352541);
location p46(-30.22209862246136, -27.09000238824278);
location p47(-39.013369620958656, -23.615017113149023);
location p48(-44.507913995019479, -26.177878943535205);
location p49(-37.835967255088491, -20.560723885721131);

location p50(-24.057753870213595, 16.008441092056486);
location p51(-22.274615173704827, 18.186048796680481);
location p52(-25.840892566722367, 17.714844402439169);
location p53(-26.138082349473823, 13.999151530674069);
location p54(-21.184919303616134, 12.456244037961326);
location p55(-23.760564087462139, 14.670999206436761);

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

std::vector<location> test_points_2 = {
  p10,
  p11,
  p12,
  p13,
  p14,
};

std::vector<location> test_points_3 = {
  p20, p21, p22, p23, p24,
};

std::vector<location> test_points_4 = {
  p30, p31, p32, p33, p34,
};

std::vector<location> test_points_5 = {
  p40, p41, p42, p43, p44, p45, p46, p47, p48, p49,
};

std::vector<location> test_points_6 = {
  p50, p51, p52, p53, p54, p55,
};

// Tests the example for anti-meridian cutting from the RFC.
bool test_rfc7946_example()
{
  std::vector<location> test_path;
  location p1(1, 170.0, 45.0);
  location p2(2, -170.0, 45.0);
  test_path.push_back(p1);
  test_path.push_back(p2);
  GeoMapUtils g;
  g.add_path(test_path.begin(), test_path.end());
  const auto result = g.as_geojson();
  bool retval = result.dump() == "{\"type\":\"MultiLineString\",\"coordinates\":"
    "[[[170.0,45.0],[180.0,45.0]],"
    "[[-180.0,45.0],[-170.0,45.0]]]}";
  if (!retval) {
    std::cerr << "test_rfc7946_example() failed\n" << result.dump() << '\n';
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
  GeoMapUtils g;
  g.add_path(test_path.begin(), test_path.end());
  const auto result = g.as_geojson();
  const auto expected = "{\"type\":\"LineString\",\"coordinates\":[[10.0,45.0],[20.0,43.0],[25.0,61.0]]}";
  auto retval = result.dump() == expected;
  if (!retval)
    std::cerr << "test_linestring() failed\n"
              << "Expected: " << expected
              << "\n but was: " << result.dump() << '\n';
  return retval;
}

// Simple test for a Point
bool test_point()
{
  std::vector<location> test_path;
  test_path.push_back(location(1, 10.0, 45.0));
  GeoMapUtils g;
  g.add_path(test_path.begin(), test_path.end());
  const auto result = g.as_geojson();
  auto retval = result.dump() == "{\"type\":\"Point\",\"coordinates\":[10.0,45.0]}";
  if (!retval)
    std::cerr << "test_point() failed\n";
  return retval;
}

bool test_empty_path()
{
  std::vector<location> test_path;
  GeoMapUtils g;
  g.add_path(test_path.begin(), test_path.end());
  const auto result = g.as_geojson();
  bool retval = result.dump() == "null";
  if (!retval) {
    std::cerr << "test_empty_path() failed\n" << result.dump() << '\n';
  }
  return retval;
}

bool test_degrees_to_radians()
{
  const auto r1 = GeoUtils::degrees_to_radians(1);
  bool retval = std::round(std::abs(r1 - 0.01745329252) * 1e10) == 0;
  if (!retval) {
    std::cerr << "test_degrees_to_radians() failed.  "
      "Expected approximately 0.01745329252, but was "
              << std::fixed << std::setprecision(12)
              << r1 << '\n';
  }
  const auto r2 = GeoUtils::degrees_to_radians(45);
  retval = retval && std::round(std::abs(r2 - 0.785398163397448) * 1e10) == 0;
  if (!retval) {
    std::cerr << "test_degrees_to_radians() failed.  "
      "Expected approximately 0.785398163397448, but was "
              << std::fixed << std::setprecision(12)
              << r2 << '\n';
  }
  return retval;
}

bool test_distance()
{
  const location eiffel_tower(0, 2.2945, 48.85822222);
  const location bt_tower(0, -0.1389, 51.5215);
  const auto distance = GeoUtils::distance(eiffel_tower, bt_tower);

  bool retval = std::abs(distance - 343.047) < 1;
  if (!retval) {
    std::cerr << "test_distance() failed, expected distance to be "
      "approximtely 343.047 kilometers, but was "
              << std::fixed << std::setprecision(3)
              << distance << " metres\n";
  }
  // Reversing the order of the points should produce the same answer
  const auto distance2 = GeoUtils::distance(bt_tower, eiffel_tower);
  retval = retval && std::abs(distance2 - distance) < 1;
  if (!retval) {
    std::cerr << "test_distance() (reversed) failed, expected distance to be "
      "approximtely " << distance << " kilometers, but was "
              << std::fixed << std::setprecision(3)
              << distance2 << " metres\n";
  }
  return retval;
}

bool test_stats_single_leg_ascent()
{
  std::vector<location> points = {
    p1,
    p2
  };
  GeoStatistics geo;
  auto stats = geo.add_path(points.begin(), points.end());
  bool retval = stats.ascent.has_value() && stats.ascent.value() == 10 &&
    !stats.descent.has_value() &&
    stats.lowest.has_value() && stats.lowest.value() == 100 &&
    stats.highest.has_value() && stats.highest.value() == 110 &&
    stats.distance.has_value() && stats.distance.value() > 0;
  if (!retval)
    std::cerr << "test_stats_single_leg_ascent() failed:\n" << stats << '\n';
  return retval;
}

bool test_stats_single_leg_descent()
{
  std::vector<location> points = {
    p2,
    p1,
  };
  GeoStatistics geo;
  auto stats = geo.add_path(points.begin(), points.end());
  bool retval = !stats.ascent.has_value() &&
    stats.descent.has_value() && stats.descent.value() == 10 &&
    stats.lowest.has_value() && stats.lowest.value() == 100 &&
    stats.highest.has_value() && stats.highest.value() == 110 &&
    stats.distance.has_value() && stats.distance.value() > 0;
  if (!retval)
    std::cerr << "test_stats_single_leg_descent() failed:\n" << stats << '\n';
  return retval;
}

bool test_stats()
{
  GeoStatistics geo;
  auto stats = geo.add_path(test_points_1.begin(), test_points_1.end());
  bool retval = stats.ascent.has_value() && stats.ascent.value() == 190 &&
    stats.descent.has_value() && stats.descent.value() == 260 &&
    stats.lowest.has_value() && stats.lowest.value() == 30 &&
    stats.highest.has_value() && stats.highest.value() == 200 &&
    stats.distance.has_value() && stats.distance.value() > 0 &&
    geo.get_ascent().has_value() && geo.get_ascent().value() == stats.ascent.value() &&
    geo.get_descent().has_value() && geo.get_descent().value() == stats.descent.value() &&
    geo.get_lowest().has_value() && geo.get_lowest().value() == stats.lowest.value() &&
    geo.get_highest().has_value() && geo.get_highest().value() == stats.highest.value() &&
    geo.get_distance().has_value() && geo.get_distance().value() == stats.distance.value();
  if (!retval)
    std::cerr << "test_stats() failed:\n" << stats << '\n'
              << geo << '\n';
  return retval;
}

bool test_stats_distance()
{
  GeoStatistics geo;
  auto stats = geo.add_path(test_points_2.begin(), test_points_2.end());
  bool retval = stats.ascent.has_value() && stats.ascent.value() == 26 &&
    stats.descent.has_value() && stats.descent.value() == 30 &&
    stats.lowest.has_value() && stats.lowest.value() == 38 &&
    stats.highest.has_value() && stats.highest.value() == 68 &&
    stats.distance.has_value() && stats.distance.value() > 0 &&
    geo.get_ascent().has_value() && geo.get_ascent().value() == stats.ascent.value() &&
    geo.get_descent().has_value() && geo.get_descent().value() == stats.descent.value() &&
    geo.get_lowest().has_value() && geo.get_lowest().value() == stats.lowest.value() &&
    geo.get_highest().has_value() && geo.get_highest().value() == stats.highest.value() &&
    geo.get_distance().has_value() && geo.get_distance().value() == stats.distance.value() &&
    std::abs(std::round((geo.get_distance().value() - 11.829) * 1000) / 1000) < 1;
  if (!retval)
    std::cerr << "test_stats_distance() failed:\n" << stats << '\n' << std::round(geo.get_distance().value()) << '\n';
  return retval;
}

bool test_stats_distance_multi_path()
{
  GeoStatistics geo;
  std::vector<location> part1;
  std::vector<location> part2;
  auto i = test_points_2.begin();
  part1.push_back(*i++);
  part1.push_back(*i++);
  part2.push_back(*i++);
  part2.push_back(*i++);
  part2.push_back(*i++);
  auto stats1 = geo.add_path(part1.begin(), part1.end());
  auto stats2 = geo.add_path(part2.begin(), part2.end());
  bool retval = stats1.ascent.has_value() && stats1.ascent.value() == 26 &&
    !stats1.descent.has_value() &&
    stats1.lowest.has_value() && stats1.lowest.value() == 42 &&
    stats1.highest.has_value() && stats1.highest.value() == 68 &&
    stats1.distance.has_value() && stats1.distance.value() > 0 &&
    !stats2.ascent.has_value() &&
    stats2.descent.has_value() && stats2.descent.value() == 4 &&
    stats2.lowest.has_value() && stats2.lowest.value() == 38 &&
    stats2.highest.has_value() && stats2.highest.value() == 42 &&
    stats2.distance.has_value() && stats2.distance.value() > 0 &&
    geo.get_ascent().has_value() && geo.get_ascent().value() == 26 &&
    geo.get_descent().has_value() && geo.get_descent().value() == 30 &&
    geo.get_lowest().has_value() && geo.get_lowest().value() == 38 &&
    geo.get_highest().has_value() && geo.get_highest().value() == 68 &&
    geo.get_distance().has_value() &&
    std::abs(std::round(geo.get_distance().value()
                        - stats1.distance.value()
                        - 3.22847 // The distance between the first and second paths
                        - stats2.distance.value())) < 1 &&
    std::abs(std::round((geo.get_distance().value() - 11.829) * 1000) / 1000) < 1;
  if (!retval)
    std::cerr << "test_stats_distance_multi_path() failed:\n" << "stats1: " << stats1 << '\n'
              << "stats2: " << stats2 << '\n'
              << "Difference in total of separate distances: "
              << std::abs(std::round(geo.get_distance().value()
                                     - stats1.distance.value()
                                     - stats2.distance.value())) << '\n'
              << "overall distance: "
              << std::round(geo.get_distance().value()) << '\n';
  return retval;
}

bool test_extend_bounding_box_01()
{
  bounding_box box(p20);
  for (const auto p : test_points_3)
    box.extend(p);
  auto center = box.get_center();
  bool retval =
    std::round(std::abs(box.top_left.longitude - p20.longitude) * 1e10) == 0 &&
    std::round(std::abs(box.top_left.latitude - p22.latitude) * 1e10) == 0 &&
    std::round(std::abs(box.bottom_right.longitude - p22.longitude) * 1e10) == 0 &&
    std::round(std::abs(box.bottom_right.latitude - p20.latitude) * 1e10) == 0;
  if (retval) {
    retval =
      std::round(std::abs(center.longitude - 3.416894471723) * 1e10) == 0 &&
      std::round(std::abs(center.latitude - 5.025616239646) * 1e10) == 0;
    if (!retval)
      std::cerr << "text_extend_bounding_box_01 failed\n"
                << std::fixed << std::setprecision(12)
                << "lng: " << center.longitude
                << " lat: " << center.latitude << '\n';
  }
  return retval;
}

bool test_extend_bounding_box_02()
{
  bounding_box box(p30);
  for (const auto p : test_points_4)
    box.extend(p);
  auto center = box.get_center();
  bool retval =
    std::round(std::abs(box.top_left.longitude - p34.longitude) * 1e10) == 0 &&
    std::round(std::abs(box.top_left.latitude - p34.latitude) * 1e10) == 0 &&
    std::round(std::abs(box.bottom_right.longitude - p31.longitude) * 1e10) == 0 &&
    std::round(std::abs(box.bottom_right.latitude - p31.latitude) * 1e10) == 0;
  if (!retval) {
    std::cerr << "text_extend_bounding_box_02 failed on creating bounding box\n"
              << box << '\n';;
  } else {
    retval =
      std::round(std::abs(center.longitude - 21.859837392634) * 1e10) == 0 &&
      std::round(std::abs(center.latitude - 21.755050950708) * 1e10) == 0;
    if (!retval)
      std::cerr << "text_extend_bounding_box_02 failed on getting center\n"
                << std::fixed << std::setprecision(12)
                << "lng: " << center.longitude
                << " lat: " << center.latitude << '\n';
  }
  return retval;
}

bool test_extend_bounding_box_03()
{
  bounding_box box(p40);
  for (const auto p : test_points_5)
    box.extend(p);
  auto center = box.get_center();
  bool retval =
    std::round(std::abs(box.top_left.longitude - p40.longitude) * 1e10) == 0 &&
    std::round(std::abs(box.top_left.latitude - p41.latitude) * 1e10) == 0 &&
    std::round(std::abs(box.bottom_right.longitude - p41.longitude) * 1e10) == 0 &&
    std::round(std::abs(box.bottom_right.latitude - p46.latitude) * 1e10) == 0;
  if (!retval) {
    std::cerr << "text_extend_bounding_box_03 failed on creating bounding box\n"
              << box << '\n';;
  } else {
    retval =
      std::round(std::abs(center.longitude - -35.284928795703) * 1e10) == 0 &&
      std::round(std::abs(center.latitude -  -19.343030103383) * 1e10) == 0;
    if (!retval)
      std::cerr << "text_extend_bounding_box_03 failed on getting center\n"
                << std::fixed << std::setprecision(12)
                << "lng: " << center.longitude
                << " lat: " << center.latitude << '\n';
  }
  return retval;
}

bool test_extend_bounding_box_04()
{
  bounding_box box(p50);
  for (const auto p : test_points_6)
    box.extend(p);
  auto center = box.get_center();
  bool retval =
    std::round(std::abs(box.top_left.longitude - p53.longitude) * 1e10) == 0 &&
    std::round(std::abs(box.top_left.latitude - p51.latitude) * 1e10) == 0 &&
    std::round(std::abs(box.bottom_right.longitude - p54.longitude) * 1e10) == 0 &&
    std::round(std::abs(box.bottom_right.latitude - p54.latitude) * 1e10) == 0;
  if (!retval) {
    std::cerr << "text_extend_bounding_box_04 failed on creating bounding box\n"
              << box << '\n';;
  } else {
    retval =
      std::round(std::abs(center.longitude - -23.661500826545) * 1e10) == 0 &&
      std::round(std::abs(center.latitude -  15.321146417321) * 1e10) == 0;
    if (!retval)
      std::cerr << "text_extend_bounding_box_04 failed on getting center\n"
                << std::fixed << std::setprecision(12)
                << "lng: " << center.longitude
                << " lat: " << center.latitude << '\n';
  }
  return retval;
}

bool test_bearing_to_azimuth_01() {
  // https://mapscaping.com/how-to-calculate-bearing-between-two-coordinates/
  // https://www.igismap.com/formula-to-find-bearing-or-heading-angle-between-two-points-latitude-longitude/
  location kansas_city(-94.581213, 39.099912);
  location st_louis(-90.200203, 38.627089);
  auto bearing = GeoUtils::bearing_to_azimuth(kansas_city, st_louis);
  const bool retval = std::round(std::abs(bearing - 96.51) * 1e2) == 0;
  if (!retval)
    std::cerr << "test_bearing_to_azimuth_01 failed: "
      "expected bearing of 96.51 but was "
              << std::fixed << std::setprecision(2) << bearing << '\n';
  return retval;
}

bool test_bearing_to_azimuth_02() {
  // https://mapscaping.com/how-to-calculate-bearing-between-two-coordinates/
  // https://www.igismap.com/formula-to-find-bearing-or-heading-angle-between-two-points-latitude-longitude/
  location kansas_city(-94.581213, 39.099912);
  location st_louis(-90.200203, 38.627089);
  auto bearing = GeoUtils::bearing_to_azimuth(st_louis, kansas_city);
  const bool retval = std::round(std::abs(bearing - 279.26) * 1e2) == 0;
  if (!retval)
    std::cerr << "test_bearing_to_azimuth_02 failed: "
      "expected bearing of 279.26 but was "
              << std::fixed << std::setprecision(2) << bearing << '\n';
  return retval;
}

int main(void)
{
  try {
    return !(
        test_rfc7946_example() &&
        test_linestring() &&
        test_point() &&
        test_empty_path() &&
        test_degrees_to_radians() &&
        test_distance() &&
        test_stats_single_leg_ascent() &&
        test_stats_single_leg_descent() &&
        test_stats() &&
        test_stats_distance() &&
        test_stats_distance_multi_path() &&
        test_extend_bounding_box_01() &&
        test_extend_bounding_box_02() &&
        test_extend_bounding_box_03() &&
        test_extend_bounding_box_04() &&
        test_bearing_to_azimuth_01() &&
        test_bearing_to_azimuth_02()
      );
  } catch (const std::exception &e) {
    std::cerr << "Tests failed with: " << e.what() << '\n';
    return 1;
  }
}
