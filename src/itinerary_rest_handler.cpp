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
#include "itinerary_rest_handler.hpp"
#ifdef HAVE_GDAL
#include "elevation_tile.hpp"
#endif
#include "../trip-server-common/src/http_request.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <algorithm>
#include <map>
#include <sstream>

#ifdef USE_BOOST_SIMPLIFY
using namespace boost::geometry;
#endif
using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

#ifdef HAVE_GDAL
extern ElevationService *elevation_service;
#endif

nlohmann::basic_json<nlohmann::ordered_map>
    ItineraryRestHandler::get_routes_as_geojson(
    const std::vector<ItineraryPgDao::route> &routes) const
{
  json json_features = json::array();
  for (const auto &r : routes) {
    GeoMapUtils geoRoute;
    geoRoute.add_path(r.points.begin(), r.points.end());
    json properties;
    json feature{
      {"type", "Feature"},
      {"geometry", geoRoute.as_geojson()},
    };
    properties["type"] = "route";
    if (r.id.first)
      properties["id"] = r.id.second;
    if (r.name.first)
      properties["name"] = r.name.second;
    if (r.color_key.first)
      properties["color_code"] = r.color_key.second;
    if (r.html_code.first)
      properties["html_color_code"] = r.html_code.second;
    feature["properties"] = properties;
    json_features.push_back(feature);
  }
  return json_features;
}

nlohmann::basic_json<nlohmann::ordered_map>
    ItineraryRestHandler::get_tracks_as_geojson(
        const std::vector<ItineraryPgDao::track> &tracks
      ) const {
  json json_features = json::array();
  for (const auto &t : tracks) {
    GeoMapUtils geoTrack;
    for (const auto &ts : t.segments) {
      geoTrack.add_path(ts.points.begin(), ts.points.end());
    }
    json properties;
    json feature{
      {"type", "Feature"},
      {"geometry", geoTrack.as_geojson()},
    };
    properties["type"] = "track";
    if (t.id.first)
      properties["id"] = t.id.second;
    if (t.name.first)
      properties["name"] = t.name.second;
    if (t.html_code.first)
      properties["html_color_code"] = t.html_code.second;
    if (t.color_key.first)
      properties["color_code"] = t.color_key.second;
    feature["properties"] = properties;
    json_features.push_back(feature);
  }
  return json_features;
}

nlohmann::basic_json<nlohmann::ordered_map>
    ItineraryRestHandler::get_track_segments_as_geojson(
        const ItineraryPgDao::path_summary &track,
        const std::vector<ItineraryPgDao::track_segment> &segments) const
{
  json json_features = json::array();
  for (const auto &ts : segments) {
    GeoMapUtils geoTrack;
    geoTrack.add_path(ts.points.begin(), ts.points.end());
    json properties{
      {"type", "track"}
    };
    if (track.id.first)
      properties["track_id"] = track.id.second;
    if (ts.id.first)
      properties["id"] = ts.id.second;
    if (track.html_code.first)
      properties["html_color_code"] = track.html_code.second;
    if (track.color_key.first)
      properties["color_code"] = track.color_key.second;
    json feature{
      {"type", "Feature"},
      {"properties", properties}
    };
    feature["geometry"] = geoTrack.as_geojson();
    json_features.push_back(feature);
  }
  return json_features;
}

nlohmann::basic_json<nlohmann::ordered_map>
    ItineraryRestHandler::get_waypoints_as_geojson(
    const std::vector<ItineraryPgDao::waypoint> &waypoints) const
{
  json json_features = json::array();
  for (const auto &w : waypoints) {
    json properties;
    json feature{
      {"type", "Feature"}
    };
    json geometry;
    geometry["type"] = "Point";
    json coords{
      w.longitude,
      w.latitude
    };
    geometry["coordinates"] = coords;
    feature["geometry"] = geometry;

    properties["type"] = "waypoint";
    if (w.id.first)
      properties["id"] = w.id.second;
    if (w.name.first)
      properties["name"] = w.name.second;
    feature["properties"] = properties;
    json_features.push_back(feature);
  }
  return json_features;
}

nlohmann::basic_json<nlohmann::ordered_map>
    ItineraryRestHandler::get_track_points_as_geojson(
        const std::vector<long> &point_ids,
        ItineraryPgDao &dao) const
{
  auto points = dao.get_track_points(get_user_id(), itinerary_id, point_ids);
  json json_features = json::array();
  for (const auto &w : points) {
    json properties;
    json feature{
      {"type", "Feature"}
    };
    json geometry;
    geometry["type"] = "Point";
    json coords{
      w.longitude,
      w.latitude
    };
    geometry["coordinates"] = coords;
    feature["geometry"] = geometry;

    properties["type"] = "waypoint";
    if (w.id.first) {
      properties["id"] = w.id.second;
      std::ostringstream ss;
      ss << as::number << std::setprecision(0) << w.id.second;
      properties["name"] = ss.str();
    }
    feature["properties"] = properties;
    json_features.push_back(feature);
  }
  return json_features;
}

nlohmann::basic_json<nlohmann::ordered_map>
    ItineraryRestHandler::get_route_points_as_geojson(
        const std::vector<long> &point_ids,
        ItineraryPgDao &dao) const
{
  auto points = dao.get_route_points(get_user_id(), itinerary_id, point_ids);
  json json_features = json::array();
  for (const auto &w : points) {
    json properties;
    json feature{
      {"type", "Feature"}
    };
    json geometry;
    geometry["type"] = "Point";
    json coords{
      w.longitude,
      w.latitude
    };
    geometry["coordinates"] = coords;
    feature["geometry"] = geometry;

    properties["type"] = "waypoint";
    if (w.id.first) {
      properties["id"] = w.id.second;
      std::ostringstream ss;
      ss << as::number << std::setprecision(0) << w.id.second;
      properties["name"] = ss.str();
    }
    feature["properties"] = properties;
    json_features.push_back(feature);
  }
  return json_features;
}

void ItineraryRestHandler::fetch_itinerary_features(
    long itinerary_id,
    ItineraryPgDao::selected_feature_ids features,
    web::HTTPServerResponse &response) const
{
  ItineraryPgDao dao;
  dao.validate_user_itinerary_read_access(get_user_id(),
                                          itinerary_id);
  const auto routes = dao.get_routes(get_user_id(),
                                     itinerary_id, features.routes);
  const auto waypoints = dao.get_waypoints(get_user_id(),
                                           itinerary_id, features.waypoints);
  const auto tracks = dao.get_tracks(get_user_id(),
                                     itinerary_id, features.tracks);
  json j;
  j["routes"] = {
    {"type", "FeatureCollection"},
    {"features", get_routes_as_geojson(routes)}
  };
  j["tracks"] = {
    {"type", "FeatureCollection"},
    {"features", get_tracks_as_geojson(tracks)}
  };
  j["waypoints"] = {
    {"type", "FeatureCollection"},
    {"features", get_waypoints_as_geojson(waypoints)}
  };
  // Those nicknames that are sharing their locations
  const auto nicknames =
    dao.get_nicknames_sharing_location_with_user(get_user_id());
  j["locationSharers"] = nicknames;
  if (!nicknames.empty())
    j["pathColors"] = static_cast<nlohmann::json>(dao.get_path_colors());
  // std::cout << j.dump(4) << '\n';
  response.content << j << '\n';
}

void ItineraryRestHandler::fetch_itinerary_segments(
    const nlohmann::basic_json<nlohmann::ordered_map> &json_request,
    std::ostream &os
  ) const
{
  // std::cout << "Fetching segments for itinerary ID: " << itinerary_id << "\n";
  long track_id = json_request["track_id"].get<long>();
  std::vector<long> segment_ids = json_request["segments"].get<std::vector<long>>();
  // for (auto n : segment_ids) std::cout << n << '\n';
  ItineraryPgDao dao;
  auto track = dao.get_track_summary(get_user_id(), itinerary_id, track_id);
  auto segments = dao.get_track_segments(
      get_user_id(),
      itinerary_id,
      track_id,
      segment_ids);
  json j;
  j["tracks"] = {
    {"type", "FeatureCollection"},
    {"features", get_track_segments_as_geojson(track, segments)}
  };
  os << j << '\n';
}

void ItineraryRestHandler::fetch_itinerary_track_points(
    const nlohmann::basic_json<nlohmann::ordered_map> &json_request,
    std::ostream &os
  ) const
{
  ItineraryPgDao dao;
  long track_id = json_request["track_id"].get<long>();
  auto track = dao.get_track(get_user_id(), itinerary_id, track_id);
  std::vector<ItineraryPgDao::track> tracks = { track };
  std::vector<long> point_ids = json_request["track_point_ids"].get<std::vector<long>>();
  std::vector<long> track_ids = {track_id};
  json j;
  j["tracks"] = {
    {"type", "FeatureCollection"},
    {"features", get_tracks_as_geojson(tracks)}
  };
  j["waypoints"] = {
    {"type", "FeatureCollection"},
    {"features", get_track_points_as_geojson(point_ids, dao)}
  };
  // std::cout << "Returning features:\n" << j.dump(4) << '\n';
  os << j << '\n';
}

void ItineraryRestHandler::fetch_itinerary_route_points(
    const nlohmann::basic_json<nlohmann::ordered_map> &json_request,
    std::ostream &os
  ) const
{
  ItineraryPgDao dao;
  long route_id = json_request["route_id"].get<long>();
  auto route = dao.get_route(get_user_id(), itinerary_id, route_id);
  std::vector<ItineraryPgDao::route> routes = { route };
  std::vector<long> point_ids = json_request["route_point_ids"].get<std::vector<long>>();
  std::vector<long> route_ids = {route_id};
  json j;
  j["routes"] = {
    {"type", "FeatureCollection"},
    {"features", get_routes_as_geojson(routes)}
  };
  j["waypoints"] = {
    {"type", "FeatureCollection"},
    {"features", get_route_points_as_geojson(point_ids, dao)}
  };
  // std::cout << "Returning features:\n" << j.dump(4) << '\n';
  os << j << '\n';
}

std::vector<location> ItineraryRestHandler::get_coordinates(
    const nlohmann::basic_json<nlohmann::ordered_map> &coordinates)
{
  std::vector<location> locations;
  if (coordinates.is_array()) {
    for (json::const_iterator co_it = coordinates.begin();
         co_it != coordinates.end(); ++co_it) {
      const auto coord = *co_it;
      location loc;
      loc.longitude = coord[0];
      loc.latitude = coord[1];
      if (coord.size() > 2) {
        const auto alt_json = coord[2];
        if (alt_json.is_number()) {
          loc.altitude.second = alt_json;
          loc.altitude.first = true;
        }
      }
      locations.push_back(loc);
    }
  } else {
    throw BadRequestException("Path contains no coordinates");
  }
  return locations;
}

std::string ItineraryRestHandler::create_track_point_key(
    const location &point)
{
  std::ostringstream os;
  os << std::fixed << std::setprecision(12) << point.longitude << ','
     << point.latitude;
  return os.str();
}

void ItineraryRestHandler::save_simplified_track(
    std::vector<location> &locations,
    double tolerance,
    const nlohmann::basic_json<nlohmann::ordered_map> &properties)
{
  // In simplifying the path using OpenLayers (at at version 7.1.0) we lose the
  // altitude, hdop and time attributes of each point in the resultant GeoJSON
  // object.
  //
  // This method handles this situation by making a copy of the original path
  // and removing all points that are no longer in the simplified path.
  //
  // The points from the simplified path are added to a map.  To reduce rounding
  // precision issues, the points are keyed by a concatenation of longitude and
  // latitude rounded to a specific precision.
  //
  // Where points are very close together they survive this process, therefore,
  // after first finding a point, it is removed from the map, causing any
  // further points appearing to belong to the same location to also be removed.
  // This may be undesirable in some situations, such as a figure of 8
  // track, but otherwise difficult to avoid.

  // A map with keys used to identifying a location with a reasonable amount of
  // accuracy
  if (properties.find("id") != properties.end()) {
    const long original_track_id = properties["id"];
    // std::cout << "Original track ID: " << original_track_id << '\n';
    ItineraryPgDao dao;
    std::vector<long> track_ids;
    track_ids.push_back(original_track_id);
    auto tracks = dao.get_tracks(get_user_id(), itinerary_id, track_ids);
    if (tracks.empty())
      throw BadRequestException("Unable to find original track");
    ItineraryPgDao::itinerary_features features;
#ifndef USE_BOOST_SIMPLIFY
    std::map<std::string, ItineraryPgDao::track_point> simplified_map;
    // Create a map containing each location in the simplified track
    std::for_each(
        locations.cbegin(),
        locations.cend(),
        [&simplified_map] (const location &loc) {
          simplified_map[ItineraryRestHandler::create_track_point_key(loc)]
            = loc;
        });
    for (auto &t : tracks) {
      t.id.first = false;
      // A note appended to a path name which has been simplified
      const std::string additional_note = translate("(simplified)");
      if (t.name.first) {
        if (!(t.name.second.empty() || additional_note.empty()))
          t.name.second += " ";
        t.name.second += additional_note;
      } else {
        t.name =
          std::make_pair(true, additional_note);
      }

      for (auto &ts : t.segments) {
        if (ts.points.size() <= 2)
          continue;
        ts.id.first = false;
        for (auto &point : ts.points) {
          point.id.first = false;
        }
        // std::cout << ts.points.size() << " points in segment before processing\n";

        // Remove the first and last points from the map of simplified points,
        // if they exist, as we will not remove them during the loop, but we
        // don't want to keep any other points for the same location.
        std::string key = ItineraryRestHandler::create_track_point_key(ts.points.front());
        auto iter = simplified_map.find(key);
        if (iter != simplified_map.end())
          simplified_map.erase(iter);
        key = ItineraryRestHandler::create_track_point_key(ts.points.back());
        iter = simplified_map.find(key);
        if (iter != simplified_map.end())
          simplified_map.erase(iter);

        // Remove all the points that aren't in the map of simplified points
        ts.points.erase(
            std::remove_if(
                ts.points.begin() +1,
                ts.points.end() -1,
                [&simplified_map]
                (const ItineraryPgDao::track_point &point) {
                  const std::string key =
                    ItineraryRestHandler::create_track_point_key(point);
                  auto iter = simplified_map.find(key);
                  if (iter != simplified_map.end()) {
                    // Erase the entry from the map so any remaining points with
                    // the same locations will also be removed
                    simplified_map.erase(iter);
                    return false;
                  } else {
                    // Do not delete the first and last points
                    return true;
                  }
                }), ts.points.end() -1);

        // Clear the IDs of all the remaining points
        for (auto &point : ts.points)
          point.id.first = false;

      }
      features.tracks.push_back(t);
    }
#else
    const auto original_tolerance = tolerance;
    tolerance = 0.0001;
    // std::cout << std::fixed << std::setprecision(3)
    //           << "Original tolerance " << original_tolerance << " : "
    //           << (original_tolerance / tolerance) << '\n';
    for (auto &t : tracks) {
      ItineraryPgDao::track new_track(t);
      new_track.segments.clear();
      new_track.id.first = false;
      // A note appended to a path name which has been simplified
      const std::string additional_note = translate("(simplified)");
      if (new_track.name.first) {
        if (!(new_track.name.second.empty() || additional_note.empty()))
          new_track.name.second += " ";
        new_track.name.second += additional_note;
      } else {
        new_track.name =
          std::make_pair(true, additional_note);
      }
      for (auto &ts : t.segments) {
        std::vector<ItineraryPgDao::track_point> simplified_points;
        simplify(ts.points, simplified_points, tolerance);
        // std::cout << "Simplified segment using tolerance of "
        //           << std::fixed << std::setprecision(5) << tolerance
        //           << " points reduced from " << ts.points.size()
        //           << " to " << simplified_points.size() << " points\n";
        for (auto &point : simplified_points) {
          point.id.first = false;
        }
        ItineraryPgDao::track_segment new_segment(std::move(simplified_points));
        new_track.segments.push_back(new_segment);
      }
      // for (auto &ts : new_track.segments)
      //   std::cout << "new track segment has " << ts.points.size() << " points\n";
      features.tracks.push_back(new_track);
    }
#endif
    ItineraryPgDao::track::calculate_statistics(features.tracks);
    dao.create_itinerary_features(get_user_id(), itinerary_id, features);
  } else {
    throw BadRequestException("ID property missing for original track");
  }
}

void ItineraryRestHandler::save_simplified_feature(
    const nlohmann::basic_json<nlohmann::ordered_map> &j)
{
  // std::cout << "JSON:\n" << j.dump(4) << '\n';
  // std::cout << "Saving features for itinerary ID: "
  //           << itinerary_id << "\n";
  const auto track = j["track"];
  const std::string type = track["type"];
  const auto geometry = track["geometry"];
  const auto properties = track["properties"];
  const auto featureType = properties["type"];
  const std::string geometryFeatureType = geometry["type"];
  if (featureType.is_string() && featureType == "track") {
    std::vector<location> locations = extract_locations(track);
    if (!locations.empty()) {
      if (j.find("tolerance") == j.end())
        throw BadRequestException("tolerance value is not specified for the track");
      const double tolerance = j["tolerance"];
      save_simplified_track(locations, tolerance, properties);
    }
  }
}

std::vector<location> ItineraryRestHandler::extract_locations(
    const nlohmann::basic_json<nlohmann::ordered_map> &feature)
{
  const std::string type = feature["type"];
  if (type == "Feature") {
    std::vector<location> locations;
    const auto geometry = feature["geometry"];
    const auto properties = feature["properties"];
    const auto featureType = properties["type"];
    const std::string geometryFeatureType = geometry["type"];
    if (geometryFeatureType == "LineString") {
      // array of coordinate arrays
      locations = get_coordinates(geometry["coordinates"]);
    } else if (geometryFeatureType == "MultiLineString") {
      const auto multi_line_string = geometry["coordinates"];
      if (multi_line_string.is_array()) {
        for (json::const_iterator i = multi_line_string.begin();
             i != multi_line_string.end(); ++i) {
          const auto segment = get_coordinates(*i);
          locations.insert(locations.end(), segment.begin(), segment.end());
        }
      } else {
        throw BadRequestException("Bad format for MultiLineString coordinates");
      }
    } else {
      std::cerr << "Unexpected geometry feature type: \""
                << geometryFeatureType << "\"\n";
      throw BadRequestException("Unexpected geometry feature type");
    }
    return locations;
  } else {
    std::cerr << "Unexpected geometry type: \""
              << type << "\"\n";
    throw BadRequestException("Unexpected geometry type");
  }
}

ItineraryPgDao::route ItineraryRestHandler::create_route(
    const nlohmann::basic_json<nlohmann::ordered_map> &j)
{
  const auto j_route = j["feature"];
  auto locations = extract_locations(j_route);
  ItineraryPgDao::route route;
  const auto properties = j_route["properties"];
  if ((route.id.first = properties.find("id") != properties.end()))
    route.id.second = properties["id"];
  if ((route.name.first = properties.find("name") != properties.end()))
    route.name.second = properties["name"];
  if ((route.color_key.first = properties.find("color_code") != properties.end()))
    route.color_key.second = properties["color_code"];
  if ((route.html_code.first =
       properties.find("html_color_code") != properties.end()))
    route.html_code.second = properties["html_color_code"];

  for (const auto& location : locations)
    route.points.push_back(ItineraryPgDao::route_point(location));
#ifdef HAVE_GDAL
  // OpenLayers sets the altitude to zero when creating new points.
  // Additionally, if a point has been moved, it's altitude may change, so
  // re-calculate all altitudes where possible.
  if (elevation_service)
    elevation_service->fill_elevations(
        route.points.begin(),
        route.points.end(),
        true // use force option
      );
#endif
  route.calculate_statistics();
  return route;
}

ItineraryPgDao::waypoint ItineraryRestHandler::create_waypoint(
    const nlohmann::basic_json<nlohmann::ordered_map> &j)
{
  const auto j_waypoint = j["feature"];
  ItineraryPgDao::waypoint waypoint;
  const auto properties = j_waypoint["properties"];
  if ((waypoint.id.first = properties.find("id") != properties.end()))
    waypoint.id.second = properties["id"];
  if (!waypoint.id.first && !waypoint.time.first)
    waypoint.time = std::make_pair(true, std::chrono::system_clock::now());
  auto coordinates = j_waypoint["geometry"]["coordinates"];
  waypoint.longitude = coordinates[0];
  waypoint.latitude = coordinates[1];
#ifdef HAVE_GDAL
  if (elevation_service) {
    auto altitude = elevation_service->get_elevation(
        waypoint.longitude,
        waypoint.latitude);
    // The altitude may have been manually set by the user, so only adjust when
    // we have elevation data
    if (altitude.first)
      waypoint.altitude = altitude;
  }
#endif
  return waypoint;
}

void ItineraryRestHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response)
{
  // std::cout << "Request content: \"" << request.content << "\"\n";
  try {
    json j = json::parse(request.content);
    try {
      // std::cout << j.dump(4) << '\n';
      itinerary_id = j["itinerary_id"];
      if (request.method == HTTPMethod::post) {
        const auto action_j = j["action"];
        const std::string action = action_j.is_string() ? action_j : "";
        if (action == "save_simplified") {
          save_simplified_feature(j);
        } else if (action == "create-map-feature") {
          // std::cout << "Creating map feature\n" << j.dump(4) << '\n';
          ItineraryPgDao dao;
          const auto j_route = j["feature"];
          const auto properties = j_route["properties"];
          if (properties.find("type") != properties.end()) {
            const std::string type = properties["type"];
            if (type == "route") {
              auto route = create_route(j);
              dao.save(get_user_id(), itinerary_id, route);
              if (route.id.first)
                response.content << "{\"id\": " << route.id.second << "}\n";
            } else if (type == "waypoint") {
              auto waypoint = create_waypoint(j);
              // Only the location will have changed for an existing waypoint.
              if (waypoint.id.first) {
                auto original_waypoint = dao.get_waypoint(
                    get_user_id(),
                    itinerary_id,
                    waypoint.id.second);
                if (original_waypoint.id.first == waypoint.id.first) {
                  original_waypoint.longitude = waypoint.longitude;
                  original_waypoint.latitude = waypoint.latitude;
                  original_waypoint.altitude = waypoint.altitude;
                  waypoint = original_waypoint;
                }
              }
              dao.save(get_user_id(), itinerary_id, waypoint);
              if (waypoint.id.first)
                response.content << "{\"id\": " << waypoint.id.second << "}\n";
            } else {
              throw BadRequestException("Unexpected GeoJSON property type " + type);
            }
          } else {
            throw BadRequestException("GeoJSON type property not set");
          }
        } else if (action == "delete-features") {
          auto features = j["features"];
          // std::cout << "Features:\n" << features.dump(4) << '\n';
          ItineraryPgDao dao;
          ItineraryPgDao::selected_feature_ids selected_features(features);
          dao.delete_features(get_user_id(), itinerary_id, selected_features);
        } else if (action_j.is_null()) {
          if (j.find("features") != j.end()) {
            auto feature_info =
              j["features"].get<fdsd::trip::ItineraryPgDao::selected_feature_ids>();
            fetch_itinerary_features(itinerary_id, feature_info, response);
          } else if (j.find("segments") != j.end()) {
            fetch_itinerary_segments(j, response.content);
          } else if (j.find("track_point_ids") != j.end()) {
            // std::cout << "Fetching itinerary track points\n";
            fetch_itinerary_track_points(j, response.content);
          } else if (j.find("route_point_ids") != j.end()) {
            // std::cout << "Fetching itinerary route points\n";
            fetch_itinerary_route_points(j, response.content);
          } else {
            std::cerr << get_handler_name() << ": Unable to handle request\n";
          }
          response.set_header("Content-Type", get_mime_type("json"));
        } else {
          throw BadRequestException("Unexpected action: \"" + action + "\"");
        }
      } else {
        throw BadRequestException(
            "Itinerary rest request - unexpected request method");
      }
    } catch (const std::exception &e) {
      std::cerr << "Exception handling itinerary rest request:\n"
                << e.what() << '\n';
      throw;
    }
  } catch (const nlohmann::json::exception &e) {
    throw BadRequestException("Error parsing JSON");
  }
}
