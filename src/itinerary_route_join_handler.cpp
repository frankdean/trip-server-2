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
#include "itinerary_route_join_handler.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/uri_utils.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

void ItineraryRouteJoinHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  ItineraryPathJoinHandler::do_preview_request(request, response);
  // Title of the page for joining itinerary routes
  set_page_title(translate("Join Itinerary Routes"));
}

std::vector<ItineraryPgDao::path_summary>
    ItineraryRouteJoinHandler::get_paths(ItineraryPgDao &dao)
{
  return dao.get_route_summaries(get_user_id(), itinerary_id, path_ids);
}

void ItineraryRouteJoinHandler::join_paths(ItineraryPgDao &dao,
                                           std::vector<long> ids)
{
  const auto routes = dao.get_routes(get_user_id(), itinerary_id, ids);

  // Re-order the paths read from the database to match the user specified order
  // for joining
  std::map<long, ItineraryPgDao::route> temp_map;
  for (const auto &r : routes)
    if (r.id.has_value())
      temp_map[r.id.value()] = r;
  std::vector<ItineraryPgDao::route> ordered_routes;
  for (const auto &p : posted_paths_map)
    ordered_routes.push_back(temp_map.at(std::stol(p.second)));

  ItineraryPgDao::route joined_route;
  if (joined_path_name) {
    joined_route.name = joined_path_name;
  }
  joined_route.color_key = joined_path_color_key;

  for (const auto &r : ordered_routes)
    for (const auto &p : r.points)
      joined_route.points.push_back(p);

  joined_route.calculate_statistics();
  dao.save(get_user_id(), itinerary_id, joined_route);
}

ItineraryPgDao::selected_feature_ids
    ItineraryRouteJoinHandler::get_selected_features()
{
  ItineraryPgDao::selected_feature_ids features;
  features.routes = path_ids;
  return features;
}
