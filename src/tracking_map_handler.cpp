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
#include "tracking_map_handler.hpp"
#include "trip_config.hpp"
#include "session_pg_dao.hpp"
#include "tracking_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>
#include <syslog.h>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using json = nlohmann::json;

const std::string TrackingMapHandler::tracking_map_url = "/map";

TrackingMapHandler::TrackingMapHandler(std::shared_ptr<TripConfig> config) :
  BaseMapHandler(config)
{
}

std::string TrackingMapHandler::get_page_title() const
{
  return "Tracks Map";
}

bool TrackingMapHandler::can_handle(
    const web::HTTPServerRequest& request) const
{
  const std::string wanted_url = get_uri_prefix() + tracking_map_url;
  return !request.uri.empty() &&
    request.uri.compare(0, wanted_url.length(), wanted_url) == 0;
}

void TrackingMapHandler::append_pre_body_end(std::ostream& os) const
{
  BaseMapHandler::append_pre_body_end(os);
  os << "    <script type=\"module\" src=\"" << get_uri_prefix() << "/static/js/tracking-map.js\"></script>\n";
}

void TrackingMapHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  // for (auto const& p : request.query_params) {
  //   std::cout << p.first << " -> " << p.second << '\n';
  // }

  const std::string latitude = request.get_query_param("lat");
  const std::string longitude = request.get_query_param("lng");
  const bool is_map_point_query = !(latitude.empty() || longitude.empty());
  if (!is_map_point_query) {
    // Save the query parameters
    SessionPgDao session_dao;
    TrackPgDao::location_search_query_params q(get_user_id(),
                                               request.get_query_params());
    json j = q;
    // This is user supplied data, so serialization could fail
    try {
      // std::cout << "Saving JSON: " << j.dump(4) << '\n';
      session_dao.save_value(get_session_id(),
                             SessionPgDao::tracks_query_key,
                             j.dump());
    } catch (const std::exception& e) {
      std::cerr << "Failed to save query parameters in session: "
                << e.what() << '\n';
      syslog(LOG_ERR, "Failed to save query parameters in session: %s",
             e.what());
      throw;
    }
  }
  response.content <<
    "    <div id=\"map\"></div>\n"
    "    <div id=\"popup\" class=\"ol-popup\">\n"
    "      <a href=\"#\" id=\"popup-closer\" class=\"ol-popup-closer\"></a>\n"
    "      <div id=\"popup-content\"></div>\n"
    "    </div>\n";
  // The #track-info div is used to display metrics for the track
  if (!is_map_point_query)
    response.content << "    <div id=\"track-info\"></div>\n";
  response.content <<
    "    <script>\n"
    "      <!--\n"
    "      const server_prefix = '" << get_uri_prefix() << "'\n"
    // Text displayed to user after clicking on map exit button to exit map
    "      const click_to_exit_text = '" << translate("Click to exit") << "';\n";
  append_map_provider_configuration(response.content);
  response.content <<
    "      // -->\n"
    "    </script>\n";
}
