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
#include "tracking_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <limits>
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

const std::string TrackingRestHandler::handled_url = "/rest/locations";

TrackingRestHandler::TrackingRestHandler(std::shared_ptr<TripConfig> config) :
    BaseRestHandler(config)
{
}

void TrackingRestHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response) const
{
  // std::cout << "TrackingRestHandler::handle_authenticated_request()\n";

  TrackPgDao dao;
  TrackPgDao::location_search_query_params q(get_user_id(),
                                             request.query_params);
  q.order = dao_helper::ascending;
  q.page_offset = -1;
  const int max_result_count = 1000;
  q.page_size = max_result_count;
  TrackPgDao::tracked_locations_result locations_result = dao.get_tracked_locations(q);

  if (!locations_result.locations.empty()) {
    GeoUtils geoUtils;
    geoUtils.add_path(locations_result.locations.begin(), locations_result.locations.end());
    json feature {
      {"type", "Feature"},
      {"geometry", geoUtils.as_geojson()}
    };
    json features;
    features.push_back(feature);
    json j{
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

    auto most_recent = locations_result.locations.back();
    std::ostringstream time;
    time << as::datetime << most_recent.time;
    json marker {
      {"position",
       { std::round(most_recent.longitude * 1e6) / 1e6,
         std::round(most_recent.latitude * 1e6) / 1e6 }
      },
      {"time", time.str()}
    };
    if (!most_recent.note.empty()) {
      marker.push_back({"note", x(most_recent.note)});
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
                // maxium amount of locations that can be displayed on the map
                "The {1} locations have been truncated to the first {2}"
              )) % locations_result.total_count % max_result_count;
      j["message"] = message.str();
    }
    // std::cout << j.dump(4) << std::endl;
    response.content << j.dump();
  } else {
    response.content << "{}";
  }
  response.set_header("Content-Type", get_mime_type("json"));
}
