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

using namespace fdsd::trip;
using namespace fdsd::web;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

nlohmann::basic_json<nlohmann::ordered_map>
    ItineraryRestHandler::get_routes_as_geojson(
    const std::vector<std::unique_ptr<ItineraryPgDao::route>> &routes) const
{
  json json_features = json::array();
  for (const auto &r : routes) {
    GeoMapUtils geoRoute;
    geoRoute.add_path(r->points.begin(), r->points.end());
    json properties;
    json feature{
      {"type", "Feature"},
      {"geometry", geoRoute.as_geojson()},
    };
    properties["type"] = "route";
    if (r->id.first)
      properties["id"] = r->id.second;
    if (r->name.first)
      properties["name"] = r->name.second;
    if (r->html_code.first)
      properties["html_color_code"] = r->html_code.second;
    feature["properties"] = properties;
    json_features.push_back(feature);
  }
  return json_features;
}

nlohmann::basic_json<nlohmann::ordered_map>
    ItineraryRestHandler::get_tracks_as_geojson(
        const std::vector<std::unique_ptr<ItineraryPgDao::track>> &tracks
      ) const {
  json json_features = json::array();
  for (const auto &t : tracks) {
    GeoMapUtils geoTrack;
    for (const auto &ts : t->segments) {
      geoTrack.add_path(ts->points.begin(), ts->points.end());
    }
    json properties;
    json feature{
      {"type", "Feature"},
      {"geometry", geoTrack.as_geojson()},
    };
    properties["type"] = "track";
    if (t->id.first)
      properties["id"] = t->id.second;
    if (t->name.first)
      properties["name"] = t->name.second;
    if (t->html_code.first)
      properties["html_color_code"] = t->html_code.second;
    feature["properties"] = properties;
    json_features.push_back(feature);
  }
  return json_features;
}

nlohmann::basic_json<nlohmann::ordered_map>
    ItineraryRestHandler::get_waypoints_as_geojson(
    const std::vector<std::unique_ptr<ItineraryPgDao::waypoint>> &waypoints) const
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
      w->longitude,
      w->latitude
    };
    geometry["coordinates"] = coords;
    feature["geometry"] = geometry;

    properties["type"] = "waypoint";
    if (w->id.first)
      properties["id"] = w->id.second;
    if (w->name.first)
      properties["name"] = w->name.second;
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
  response.content << j << '\n';
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
      long itinerary_id = j["itinerary_id"];
      auto feature_info =
        j["features"].get<fdsd::trip::ItineraryPgDao::selected_feature_ids>();
      fetch_itinerary_features(itinerary_id, feature_info, response);
      response.set_header("Content-Type", get_mime_type("json"));
    } catch (const std::exception &e) {
      std::cerr << "Exception handling itinerary rest request:\n"
                << e.what() << '\n';
      throw;
    }
  } catch (const nlohmann::json::exception &e) {
    throw BadRequestException("Error parsing JSON");
  }
}
