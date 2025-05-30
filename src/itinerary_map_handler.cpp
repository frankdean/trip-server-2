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
#include "itinerary_map_handler.hpp"
#include "itinerary_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include "tracking_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace boost::locale;
using json = nlohmann::json;

std::string ItineraryMapHandler::get_page_title() const
{
  return "Itinerary Map";
}

bool ItineraryMapHandler::can_handle(
    const web::HTTPServerRequest& request) const
{
  return compare_request_regex(request.uri, "/itinerary-map($|\\?.*)");
}

void ItineraryMapHandler::append_pre_body_end(std::ostream& os) const
{
  BaseMapHandler::append_pre_body_end(os);
  os << "    <script type=\"module\" src=\"" << get_uri_prefix()
     << "/static/js/itinerary-map.js\"></script>\n";
}

void ItineraryMapHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  try {
    itinerary_id = std::stol(request.get_param("id"));
    ItineraryPgDao dao;
    TrackPgDao track_dao(elevation_service);
    const bool read_only = !dao.has_user_itinerary_modification_access(
        get_user_id(),
        itinerary_id);
    json features = ItineraryHandler::get_selected_feature_ids(request);
    json j{
      {"itinerary_id", itinerary_id},
      {"features", features}
    };
    // std::cout << j << '\n';
    response.content <<
      "    <div id=\"map\"></div>\n";
    //   "    <div id=\"popup\" class=\"ol-popup\">\n"
    //   "      <a href=\"#\" id=\"popup-closer\" class=\"ol-popup-closer\"></a>\n"
    //   "      <div id=\"popup-content\"></div>\n"
    //   "    </div>\n";
    response.content <<
      "    <script>\n"
      "      <!--\n"
      // "      const pageInfo = JSON.parse('" << j << "');\n"
      "      const readOnly = " << (read_only ? "true" : "false") << ";\n"
      "      const pageInfoJSON = '" << j << "';\n"
      "      const server_prefix = '" << get_uri_prefix() << "';\n"
      "      const logging_uuid = '" << track_dao.get_logging_uuid_by_user_id(get_user_id()) << "';\n"
      // Text displayed to user after clicking on map exit button to exit map
      "      const click_to_exit_text = '" << translate("Click to exit") << "';\n";
    append_map_provider_configuration(response.content);
    response.content <<
      "      // -->\n"
      "    </script>\n";
  } catch (const std::exception &e) {
    std::cerr << "Exception showing itinerary map: "
              << e.what() << '\n';
  }
}
