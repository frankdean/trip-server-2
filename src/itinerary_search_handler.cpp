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
#include "itinerary_search_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include "itinerary_search_results_handler.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;
using json = nlohmann::json;

void ItinerarySearchHandler::build_form(std::ostream &os)
{
  os
    <<
    "<div class=\"container-fluid\">\n"
    "<h1>" << get_page_title() << "</h1>\n"
    "  <form name=\"form\" method=\"post\" action=\"" << get_uri_prefix() << "/itinerary-search-result\">\n"
    "    <div class=\"container-fluid bg-light row g-3 my-3 pb-3 mx-0\">\n"
    "      <div class=\"col-12\">\n"
    "        <input id=\"input-lng\" type=\"hidden\" name=\"lng\">\n"
    "        <input id=\"input-lat\" type=\"hidden\" name=\"lat\">\n"
    "      </div>\n"
    "      <div class=\"col-sm-8 col-lg-4\">\n"
    // Label for input of a location in a variety of formats
    "        <label for=\"input-position\">" << x(translate("Position")) << "</label>\n"
    "        <input id=\"input-position\" name=\"position\" type=\"text\" size=\"30\" value=\"";
  os << position << "\"";
  os
    <<
    " required>\n"
    "      </div>\n"
    "      <div class=\"col-sm-6 col-lg-4\">\n"
    // Label for selecting the coordinate formatting of a location
    "        <label for=\"input-coord-format\">" << translate("Display position format") << "</label>\n"
    "        <select id=\"input-coord-format\" name=\"coordFormat\" ";
  os << ">\n";
  for (const auto &georef_format : georef_formats) {
    os
      <<
      "          <option value=\"" << x(georef_format.first) << "\"";
    append_element_selected_flag(os, georef_format.first == coord_format);
    os
      <<
      ">" << x(georef_format.second) << "</option>\n";
  }
  os
    <<
    "        </select>\n"
    "      </div>\n"
    // "      <div ng-show=\"coordFormat !== 'plus+code' && coordFormat !== 'osgb36' && coordFormat !== 'IrishGrid' && coordFormat !== 'ITM'\">\n"
    "      <div id=\"position-style-div\" class=\"col-sm-6 col-lg-4\">\n"
    // Label for selecting the formatting style for a location
    "        <label for=\"position-separator\">" << translate("Position ordering and separator") << "</label>\n"
    "        <select id=\"position-separator\" name=\"positionFormat\"";
  os <<
    ">\n"
    "          <option value=\"lat-lng\"";
  append_element_selected_flag(os, "lat-lng" == position_format);
  os <<
    ">lat lng</option>\n"
    "          <option value=\"lat,lng\"";
  append_element_selected_flag(os, "lat,lng" == position_format);
  os <<
    ">lat,lng</option>\n"
    "          <option value=\"lng-lat\"";
  append_element_selected_flag(os, "lng-lat" == position_format);
  os <<
    ">lng lat (Proj4 reversed)</option>\n"
    "          <option value=\"lng,lat\"";
  append_element_selected_flag(os, "lng,lat" == position_format);
  os <<
    ">lng,lat (Reversed)</option>\n"
    "        </select>\n"
    "      </div>\n"
    "      <div class=\"col-sm-8 col-lg-4\">\n"
    // Label for of the radius distance to perform an itinerary search with
    "        <label for=\"input-radius\">" << translate("Distance (kilometers)") << "</label>\n"
    "        <input id=\"input-radius\" name=\"radius\" type=\"number\" step=\"0.001\" min=\"0.001\" max=\"" << ItinerarySearchResultsHandler::max_search_radius_kilometers << "\" value=\"0.1\" required>\n"
    "      </div>\n"
    "      <div class=\"col-12\"><p id=\"position-text\"></p></div>\n"
    // Label of button to execute an itinerary search by distance radius
    "      <div>\n"
    "        <button id=\"btn-search\" class=\"btn btn-lg btn-success\" accesskey=\"s\" name=\"action\" value=\"search\">" << translate("Search") << "</button>\n"
    "      </div>\n"
    "    </div>\n"
    "  </form>\n"
    "</div>\n";
}

void ItinerarySearchHandler::append_pre_body_end(std::ostream& os) const
{
  os << "    <script src=\"" << get_uri_prefix()
     << "/static/proj4js-" << PROJ4JS_VERSION << "/dist/proj4.js\"></script>\n";
  os << "    <script src=\"" << get_uri_prefix()
     << "/static/open-location-code-" << OLC_VERSION << "/openlocationcode.js\"></script>\n";
  os << "    <script type=\"module\" src=\"" << get_uri_prefix()
     << "/static/js/itinerary-waypoint.js\"></script>\n";
  TripAuthenticatedRequestHandler::append_pre_body_end(os);
}

void ItinerarySearchHandler::do_preview_request(
    const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response)
{
  (void)request; // unused
  (void)response;
  // Page title of the itinerary search by location page
  set_page_title(translate("Itinerary Search by Location"));
  set_menu_item(unknown);
}

/// extracts defaults from the user's session
void ItinerarySearchHandler::extract_session_defaults(
    ItineraryPgDao &itinerary_dao)
{
  SessionPgDao session_dao;
  session_dao.remove_value(
      get_session_id(),
      SessionPgDao::itinerary_radius_search_page_key);
  // Get any previously copied waypoint coordinates;
  const std::string p =
    session_dao.get_value(get_session_id(),
                          SessionPgDao::itinerary_features_key);
  if (!p.empty()) {
    try {
      const json j = json::parse(p);
      const long itinerary_id = j["itinerary_id"];
      const std::vector<long> waypoints = j["waypoints"];
      if (!waypoints.empty()) {
        const auto waypoint_id = waypoints.front();
        const auto waypoint = itinerary_dao.get_waypoint(
            get_user_id(),
            itinerary_id,
            waypoint_id);
        position = std::to_string(waypoint.latitude) + "," +
          std::to_string(waypoint.longitude);
      }
    } catch (const std::exception& e) {
      std::cerr << "Error parsing itinerary features parameters from session: "
                << e.what() << '\n';
    }
  }
  // Get any previously saved coordinate format configuration
  const std::string json_str =
    session_dao.get_value(get_session_id(),
                          SessionPgDao::coordinate_format_key);
  if (!json_str.empty()) {
    json j = json::parse(json_str);
    coord_format = j["coord_format"].get<std::string>();
    position_format = j["position_format"].get<std::string>();
  }
}

void ItinerarySearchHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  (void)request; // unused
  (void)response;
  ItineraryPgDao dao;
  extract_session_defaults(dao);
  georef_formats = dao.get_georef_formats();
  build_form(response.content);
}
