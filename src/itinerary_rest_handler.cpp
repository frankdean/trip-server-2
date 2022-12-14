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
    if (r.color.first)
      properties["color_code"] = r.color.second;
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
    if (t.color.first)
      properties["color_code"] = t.color.second;
    feature["properties"] = properties;
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

void ItineraryRestHandler::fetch_itinerary_features(
    long itinerary_id,
    ItineraryPgDao::selected_feature_ids features,
    web::HTTPServerResponse &response) const
{
  ItineraryPgDao dao;
  dao.validate_user_itinerary_modification_access(get_user_id(),
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
  // std::cout << j.dump(4) << '\n';
  response.content << j << '\n';
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
    // TODO solution determining tolerance value when simplifying a track
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
  if (type == "Feature") {
    const auto geometry = track["geometry"];
    const auto properties = track["properties"];
    const auto featureType = properties["type"];
    const std::string geometryFeatureType = geometry["type"];
    if (featureType.is_string() && featureType == "track") {
      std::vector<location> locations;
      if (geometryFeatureType == "LineString") {
        // array of coordinate arrays
        locations = get_coordinates(geometry["coordinates"]);
      } else if (geometryFeatureType == "MultiLineString") {
        // array of array of coordinate arrays
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
      if (!locations.empty()) {
        if (j.find("tolerance") == j.end())
          throw BadRequestException("tolerance value is not specified for the track");
        const double tolerance = j["tolerance"];
        save_simplified_track(locations, tolerance, properties);
      }
    }
  } else {
    std::cerr << "Unexpected geometry type: \""
              << type << "\"\n";
    throw BadRequestException("Unexpected geometry type");
  }
}

void ItineraryRestHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response)
{
  // std::cout << "Request content: \"" << request.content << "\"\n";
  try {
    json j = json::parse(request.content);
    try {
      // std::cout << j << '\n';
      itinerary_id = j["itinerary_id"];
      if (request.method == HTTPMethod::post) {
        const auto action_j = j["action"];
        const std::string action = action_j.is_string() ? action_j : "";
        if (action == "save") {
          save_simplified_feature(j);
        } else if (action_j.is_null()) {
          auto feature_info =
            j["features"].get<fdsd::trip::ItineraryPgDao::selected_feature_ids>();
          fetch_itinerary_features(itinerary_id, feature_info, response);
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
