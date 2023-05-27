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
#include "tracking_rest_handler.hpp"
#include "trip_config.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include <algorithm>
#include <limits>
#include <boost/locale.hpp>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

TrackingRestHandler::TrackingRestHandler(std::shared_ptr<TripConfig> config) :
    BaseRestHandler(config)
{
}

void TrackingRestHandler::live_map_update_check(
    const web::HTTPServerRequest& request,
    std::ostream &os,
    TrackPgDao &dao)
{
  // Query for multiple nicknames
  std::pair<bool, long> max_hdop;
  try {
    json j = json::parse(request.content);
    // std::cout << j.dump(4) << '\n';
    if (j.contains("max_hdop")) {
      const auto j_max_hdop = j["max_hdop"];
      if (j_max_hdop.is_number()) {
        max_hdop.second = j_max_hdop;
        max_hdop.first = true;
      }
    }
    DateTime since;
    if (j.contains("from")) {
      const auto j_from = j["from"];
      since = DateTime(j_from);
      // std::cout << "From " << since.get_time_as_iso8601_gmt() << '\n';
    }
    json nicknames = j["nicknames"];
    // std::cout << "Nicknames:: " << nicknames << '\n';
    std::vector<TrackPgDao::location_update_check> criteria;
    for (auto &element : nicknames) {
      TrackPgDao::location_update_check c;
      c.nickname = element["nickname"];
      if (element.contains("min_id_threshold")) {
        const auto j_min = element["min_id_threshold"];
        if (j_min.is_number()) {
          c.min_threshold_id.second = j_min;
          c.min_threshold_id.first = true;
        }
      }
      criteria.push_back(c);
    }
    dao.check_new_locations_available(get_user_id(),
                                      since.time_tp(), criteria);
    json r;
    for (const auto &c : criteria) {
      r.push_back({
          {"nickname", c.nickname},
          {"update_available", c.update_available}
        });
    }
    // std::cout << "Response:\n" << r.dump(4);
    os << r.dump();
  } catch (const std::exception &e) {
    std::cerr << "Exception handling tracking rest POST request:\n"
              << e.what() << '\n';
    throw;
  }
}

nlohmann::basic_json<nlohmann::ordered_map>
    TrackingRestHandler::fetch_live_map_updates_as_geojson(
        const web::HTTPServerRequest& request,
        std::ostream &os,
        TrackPgDao &dao)
{
  // std::cout << "Fetching live map updates as geojson\n";
  int max_hdop = -1;
  try {
    json j = json::parse(request.content);
    // std::cout << j.dump(4) << '\n';
    if (j.contains("max_hdop")) {
      const auto j_max_hdop = j["max_hdop"];
      if (j_max_hdop.is_number())
        max_hdop = j_max_hdop;
    }
    DateTime now;
    DateTime since;
    if (j.contains("from")) {
      const auto j_from = j["from"];
      since = DateTime(j_from);
      // std::cout << "From " << since.get_time_as_iso8601_gmt() << '\n';
    }
    const std::vector<std::string> nicknames = j["nicknames"];
    const auto user_nickname = dao.get_nickname_for_user_id(get_user_id());
    json r = json::array();
    for (const auto &nickname : nicknames) {
      TrackPgDao::location_search_query_params q;
      q.user_id = get_user_id();
      if (user_nickname != nickname)
        q.nickname = nickname;
      q.date_from = since.time_t();
      q.date_to = now.time_t();
      q.max_hdop = max_hdop;
      q.order = dao_helper::ascending;
      q.page_offset = -1;
      q.page_size = config->get_maximum_location_tracking_points();
      const auto locations_result = dao.get_tracked_locations(q);
      const auto track_json =
        get_tracked_locations_as_geojson(locations_result);
      r.push_back({
          {"nickname", nickname},
          {"locations", track_json}
        });
    }
    // std::cout << r.dump(4) << '\n';
    return r;
  } catch (const std::exception &e) {
    std::cerr << "Exception fetching live map updates:\n"
              << e.what() << '\n';
    throw;
  }
}

nlohmann::basic_json<nlohmann::ordered_map>
    TrackingRestHandler::get_tracked_locations_as_geojson(
        TrackPgDao::tracked_locations_result locations_result)
{
  if (locations_result.locations.empty())
    return json{};
  GeoMapUtils geoUtils;
  geoUtils.add_path(locations_result.locations.begin(),
                    locations_result.locations.end());
  // The last location can be used by the caller to request only updates more
  // recent than this last one.
  const TrackPgDao::tracked_location last_location = *(locations_result.locations.cend() -1);
  long last_location_id = last_location.id.first ?
    last_location.id.second : 0;

  // Locations are ordered by time, not ID and are not necessarily received
  // and inserted into the database in time order.  Therefore last item may
  // not have the highest ID.
  for (
      std::vector<TrackPgDao::tracked_location>::
        reverse_iterator i = locations_result.locations.rbegin();
      i != locations_result.locations.rend(); ++i) {

    if (i->id.first)
      last_location_id = std::max(last_location_id, i->id.second);
  }
  // std::cout << "Max ID: " << last_location_id << '\n';

  json properties;
  json feature {
    {"type", "Feature"},
    {"geometry", geoUtils.as_geojson()}
  };
  properties["type"] = "track";
  feature["properties"] = properties;
  json features;
  features.push_back(feature);
  auto max_result_count = config->get_maximum_location_tracking_points();
  json j{
    {"last_location_id", last_location_id},
    {"totalCount", locations_result.total_count},
    {"maxCount", max_result_count}};
  if (geoUtils.get_max_height().first)
    j.push_back({"maxHeight", std::round(geoUtils.get_max_height().second)});
  if (geoUtils.get_min_height().first)
    j.push_back({"minHeight", std::round(geoUtils.get_min_height().second)});
  if (geoUtils.get_ascent().first)
    j.push_back({"ascent", std::round(geoUtils.get_ascent().second)});
  if (geoUtils.get_descent().first)
    j.push_back({"descent", std::round(geoUtils.get_descent().second)});

  const TrackPgDao::tracked_location most_recent =
    locations_result.locations.back();
  std::ostringstream time;
  time << as::datetime <<
    std::chrono::duration_cast<std::chrono::seconds>(
        most_recent.time_point.time_since_epoch()).count();
  json marker {
    {"position",
     { std::round(most_recent.longitude * 1e6) / 1e6,
       std::round(most_recent.latitude * 1e6) / 1e6 }
    },
    {"time", time.str()}
  };
  if (most_recent.note.first && !most_recent.note.second.empty()) {
    marker.push_back({"note", x(most_recent.note.second)});
  }
  j.push_back({"most_recent", marker});

  j.push_back(
      {"geojsonObject", {
          {"type", "FeatureCollection"},
          {"features", features}
        }
      });
  if (locations_result.total_count > max_result_count) {
    std::ostringstream message;
    message << as::number <<
      format(
          translate(
              // Message displayed to the user when they attempt to exceed the
              // maxmium amount of locations that can be displayed on the map
              "The {1} locations have been truncated to the first {2}"
            )) % locations_result.total_count % max_result_count;
    j["message"] = message.str();
  }
  return j;
}


void TrackingRestHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  TrackPgDao dao;
  // std::cout << "TrackingRestHandler::handle_authenticated_request()\n";
  if (compare_request_regex(request.uri, "/rest/locations/is-updates($|\\?.*)")) {
    if (request.method == HTTPMethod::get) {
      const std::string nickname = request.get_query_param("nickname");
      const long min_id_threshold = std::stol(request.get_query_param("min_id_threshold"));
      // std::cout << "min_id: " << min_id_threshold << ", nickname: \"" << nickname << "\"\n";
      const bool have_updates =  (dao.check_new_locations_available(
                                      get_user_id(),
                                      nickname,
                                      min_id_threshold));
      // json j;
      // j["available"] = have_updates;
      json j = {{"available", have_updates}};
      // std::cout << j.dump(4) << std::endl;
      response.content << j.dump();
    } else {
      live_map_update_check(request, response.content, dao);
    }
  } else {
    if (request.method == HTTPMethod::get) {
      TrackPgDao::location_search_query_params q(get_user_id(),
                                                 request.get_query_params());
      q.order = dao_helper::ascending;
      q.page_offset = -1;
      q.page_size = config->get_maximum_location_tracking_points();
      TrackPgDao::tracked_locations_result locations_result =
        dao.get_tracked_locations(q);
      if (!locations_result.locations.empty()) {
        const auto j = get_tracked_locations_as_geojson(locations_result);
        // std::cout << j.dump(4) << std::endl;
        response.content << j.dump();
      } else {
        response.content << "{}";
      }
    } else {
      const auto j = fetch_live_map_updates_as_geojson(request, response.content, dao);
      response.content << j.dump();
    }
  }
  response.set_header("Content-Type", get_mime_type("json"));
}
