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

#ifdef HAVE_BOOST_GEOMETRY
using namespace boost::geometry;
#endif
using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

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
    if (r.id.has_value())
      properties["id"] = r.id.value();
    if (r.name.has_value())
      properties["name"] = r.name.value();
    if (r.color_key.has_value())
      properties["color_code"] = r.color_key.value();
    if (r.html_code.has_value())
      properties["html_color_code"] = r.html_code.value();
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
    if (t.id.has_value())
      properties["id"] = t.id.value();
    if (t.name.has_value())
      properties["name"] = t.name.value();
    if (t.html_code.has_value())
      properties["html_color_code"] = t.html_code.value();
    if (t.color_key.has_value())
      properties["color_code"] = t.color_key.value();
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
    if (track.id.has_value())
      properties["track_id"] = track.id.value();
    if (ts.id.has_value())
      properties["id"] = ts.id.value();
    if (track.html_code.has_value())
      properties["html_color_code"] = track.html_code.value();
    if (track.color_key.has_value())
      properties["color_code"] = track.color_key.value();
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
    if (w.id.has_value())
      properties["id"] = w.id.value();
    if (w.name.has_value())
      properties["name"] = w.name.value();
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
    if (w.id.has_value()) {
      properties["id"] = w.id.value();
      std::ostringstream ss;
      ss << as::number << std::setprecision(0) << w.id.value();
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
    if (w.id.has_value()) {
      properties["id"] = w.id.value();
      std::ostringstream ss;
      ss << as::number << std::setprecision(0) << w.id.value();
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
          loc.altitude = alt_json;
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
    const nlohmann::basic_json<nlohmann::ordered_map> &j)
{
  // In simplifying the path using OpenLayers (at at version 7.1.0) we lose the
  // altitude, hdop and time attributes of each point in the resultant GeoJSON
  // object.
  //
  const auto j_track = j["track"];
  const auto properties = j_track["properties"];
  if (j.find("tolerance") == j.end())
    throw BadRequestException("tolerance value is not specified for the track");
  const double tolerance = j["tolerance"];
  if (properties.find("id") != properties.end()) {
    const long original_track_id = properties["id"];
    // std::cout << "Original track ID: " << original_track_id << '\n';
    // const auto type = j_track["type"];
    const auto geometry = j_track["geometry"];
    const auto geometryFeatureType = geometry["type"];

    ItineraryPgDao dao;
    ItineraryPgDao::itinerary_features features;
#ifndef HAVE_BOOST_GEOMETRY
    ItineraryPgDao::track track;
    if (properties.find("name") != properties.end())
      track.name = properties["name"];
    if (properties.find("color_code") != properties.end())
      track.color_key = properties["color_code"];
    if ( properties.find("html_color_code") != properties.end())
      track.html_code = properties["html_color_code"];

    if (geometryFeatureType == "LineString") {
      const auto locations = get_coordinates(geometry["coordinates"]);
      std::vector<ItineraryPgDao::track_point> points;
      for (const auto &loc : locations)
        points.push_back(ItineraryPgDao::track_point(loc));
      ItineraryPgDao::track_segment segment(points);
      track.segments.push_back(segment);
    } else if (geometryFeatureType == "MultiLineString") {
      const auto multi_line_string = geometry["coordinates"];
      if (multi_line_string.is_array()) {
        for (json::const_iterator i = multi_line_string.begin();
             i != multi_line_string.end(); ++i) {
          const auto locations = get_coordinates(*i);
          std::vector<ItineraryPgDao::track_point> points;
          for (const auto &loc : locations)
            points.push_back(ItineraryPgDao::track_point(loc));
          ItineraryPgDao::track_segment segment(points);
          track.segments.push_back(segment);
        }
      } else {
        throw BadRequestException("Bad format for MultiLineString coordinates");
      }
    } else {
      std::cerr << "Unexpected geometry feature type: \""
                << geometryFeatureType << "\"\n";
      throw BadRequestException("Unexpected geometry feature type");
    }
    // A note appended to a path name which has been simplified
    append_name(track.name, translate("(simplified)"));
    features.tracks.push_back(track);
    // track.calculate_statistics();
    // dao.create_track(get_user_id(), itinerary_id, track);
#else
    // const auto original_tolerance = tolerance;
    // tolerance = 0.0001;
    // std::cout << std::fixed << std::setprecision(3)
    //           << "Original tolerance " << original_tolerance << " : "
    //           << (original_tolerance / tolerance) << '\n';
    std::vector<long> track_ids;
    track_ids.push_back(original_track_id);
    auto tracks = dao.get_tracks(get_user_id(), itinerary_id, track_ids);
    if (tracks.empty())
      throw BadRequestException("Unable to find original track");
    for (auto &t : tracks) {
      ItineraryPgDao::track new_track(t);
      new_track.segments.clear();
      new_track.id = std::optional<long>();
      // A note appended to a path name which has been simplified
      append_name(new_track.name, translate("(simplified)"));
      for (auto &ts : t.segments) {
        std::vector<ItineraryPgDao::track_point> simplified_points;
        simplify(ts.points, simplified_points, tolerance);
        // std::cout << "Simplified segment using tolerance of "
        //           << std::fixed << std::setprecision(5) << tolerance
        //           << " points reduced from " << ts.points.size()
        //           << " to " << simplified_points.size() << " points\n";
        for (auto &point : simplified_points) {
          point.id = std::optional<long>();
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
  save_simplified_track(j);
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
  if (properties.find("id") != properties.end())
    route.id = properties["id"];
  if (properties.find("name") != properties.end())
    route.name = properties["name"];
  if (properties.find("color_code") != properties.end())
    route.color_key = properties["color_code"];
  if (properties.find("html_color_code") != properties.end())
    route.html_code = properties["html_color_code"];

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
  if (properties.find("id") != properties.end())
    waypoint.id = properties["id"];
  if (!waypoint.id.has_value() && !waypoint.time.has_value())
    waypoint.time = std::chrono::system_clock::now();
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
    if (altitude.has_value())
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
              if (route.id.has_value())
                response.content << "{\"id\": " << route.id.value() << "}\n";
            } else if (type == "waypoint") {
              auto waypoint = create_waypoint(j);
              // Only the location will have changed for an existing waypoint.
              if (waypoint.id.has_value()) {
                auto original_waypoint = dao.get_waypoint(
                    get_user_id(),
                    itinerary_id,
                    waypoint.id.value());
                if (original_waypoint.id.has_value() == waypoint.id.has_value()) {
                  original_waypoint.longitude = waypoint.longitude;
                  original_waypoint.latitude = waypoint.latitude;
                  original_waypoint.altitude = waypoint.altitude;
                  waypoint = original_waypoint;
                }
              }
              dao.save(get_user_id(), itinerary_id, waypoint);
              if (waypoint.id.has_value())
                response.content << "{\"id\": " << waypoint.id.value() << "}\n";
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
