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
#include "itinerary_export_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

void ItineraryExportHandler::set_content_headers(HTTPServerResponse& response) const
{
  response.set_header("Content-Length", std::to_string(response.content.str().length()));
  response.set_header("Cache-Control", "no-cache");
  response.set_header("Content-Type", get_mime_type("yaml"));
  // std::time_t now = std::chrono::system_clock::to_time_t(
  //     std::chrono::system_clock::now());
  // std::tm tm = *std::localtime(&now);
  std::ostringstream filename;
  filename << "trip-itinerary-" << itinerary_id << ".yaml";
    // std::put_time(&tm, "%FT%T%z") << ".yaml";
  response.set_header("Content-Disposition", "attachment; filename=\"" +
                      filename.str() + "\"");
}

void ItineraryExportHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  itinerary_id = stol(request.get_param("id"));
  ItineraryPgDao dao;
  const auto itinerary =
    dao.get_itinerary_details_and_summary(get_user_id(), itinerary_id);
  if (!itinerary.first)
    throw BadRequestException("Itinerary not found");
  auto node =  ItineraryPgDao::itinerary_detail::encode(itinerary.second);

  if (dao.has_user_itinerary_modification_access(get_user_id(),
                                                 itinerary_id)) {
    const auto itinerary_shares = dao.get_itinerary_shares(
        get_user_id(),
        itinerary_id);
    node["itinerary_shares"] = itinerary_shares;
  } else {
    node["itinerary_shares"] = YAML::Null;
  }
  const auto routes = dao.get_routes(get_user_id(),
                                     itinerary_id);
  const auto waypoints = dao.get_waypoints(get_user_id(),
                                           itinerary_id);
  const auto tracks = dao.get_tracks(get_user_id(),
                                     itinerary_id);

  node["routes"] = routes;
  node["waypoints"] = waypoints;
  node["tracks"] = tracks;

  // std::cout << node << '\n';
  response.content << node;
}
