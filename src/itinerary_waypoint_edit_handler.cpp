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
#include "itinerary_waypoint_edit_handler.hpp"
#include "itinerary_handler.hpp"
#include "session_pg_dao.hpp"
#ifdef HAVE_GDAL
#include "elevation_tile.hpp"
#endif
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/dao_helper.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>

using namespace boost::locale;
using json = nlohmann::json;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

#ifdef HAVE_GDAL
extern ElevationService *elevation_service;
#endif

void ItineraryWaypointEditHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  // The title of the Itinerary Waypoint page
  set_page_title(translate("Itinerary Waypoint"));
  set_menu_item(unknown);
  // auto qp = request.get_query_params();
  // for (auto const &p : qp) {
  //   std::cout << "query param: \"" << p.first << "\" -> \"" << p.second << "\"\n";
  // }
  itinerary_id = std::stol(request.get_query_param("itineraryId"));
  const std::string wpt = request.get_query_param("waypointId");
  if (!wpt.empty()) {
    waypoint_id.second = std::stol(wpt);
    waypoint_id.first = true;
  }
}

void ItineraryWaypointEditHandler::append_pre_body_end(std::ostream& os) const
{
  os << "    <script src=\"" << get_uri_prefix()
     << "/static/proj4js-" << PROJ4JS_VERSION << "/dist/proj4.js\"></script>\n";
  os << "    <script src=\"" << get_uri_prefix()
     << "/static/open-location-code-" << OLC_VERSION << "/openlocationcode.js\"></script>\n";
  os << "    <script type=\"module\" src=\"" << get_uri_prefix()
     << "/static/js/itinerary-waypoint.js\"></script>\n";
  TripAuthenticatedRequestHandler::append_pre_body_end(os);
}

void ItineraryWaypointEditHandler::build_form(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response,
    ItineraryPgDao::waypoint* waypoint,
    const std::vector<std::pair<std::string, std::string>> &georef_formats,
    const std::vector<std::pair<std::string, std::string>> &waypoint_symbols)
{
  response.content
    <<
    "<div class=\"container-fluid\">\n"
    // The title of the Itinerary Waypoint page
    "  <h1>" << translate("Itinerary Waypoint") << "</h1>\n"
    "  <form name=\"form\" method=\"post\">\n"
    "    <div class=\"container-fluid bg-light row g-3 my-3 pb-3 mx-0\">\n"
    "      <div class=\"col-12\">\n"
    "        <input type=\"hidden\" name=\"itineraryId\" value=\"" << itinerary_id << "\">\n"
    "        <input type=\"hidden\" name=\"waypoint_id\" value=\"";
  append_value(response.content, waypoint->id.first, waypoint->id.second);
  response.content
    <<
    "\">\n"
    "        <input id=\"input-lng\" type=\"hidden\" name=\"lng\">\n"
    "        <input id=\"input-lat\" type=\"hidden\" name=\"lat\">\n"
    // Label for input of a waypoint name
    "        <label for=\"input-name\">" << translate("Name") << "</label>\n"
    "        <input id=\"input-name\" name=\"name\" type=\"text\" size=\"30\" value=\"";
  append_value(response.content,
               waypoint->id.first && waypoint->name.first,
               x(waypoint->name.second));
  response.content << "\"";
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    ">\n"
    "      </div>\n"
    "      <div class=\"col-sm-8 col-lg-4\">\n"
    // Label for input of a location in a variety of formats
    "        <label for=\"input-position\">" << translate("Position") << "</label>\n"
    "        <input id=\"input-position\" name=\"position\" type=\"text\" size=\"30\" value=\"";
  append_value(response.content, waypoint->id.first, waypoint->latitude);
  append_value(response.content, waypoint->id.first, ',');
  append_value(response.content, waypoint->id.first, waypoint->longitude);
  response.content << "\"";
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    " required>\n"
    // "        <div id=\"position-required\" class=\"alert alert-warning\" role=\"alert\" ng-show=\"form.position.$error.required\">Enter the position for the itinerary waypoint</div>\n"
    // "        <div id=\"position-invalid\" class=\"alert alert-warning\" role=\"alert\">Invalid latitude/longitude.  Many common formats are recognised, but try entering numeric only values, separated by a comma, e.g. 51.3,-2.3 for N5.31&deg; W2.3&deg;.  Then look at the various supported formats by changing the Display position format below.  You can use a letter 'd' instead of the &deg; symbol.  Single and double quotes are accepted for minutes and seconds.  Latitudes must be between -90 and 90 degrees.  Longitudes must be between -180 and 180 degrees.</div>\n"
    "      </div>\n"
    "      <div class=\"col-sm-6 col-lg-4\">\n"
    // Label for selecting the coordinate formatting of a location
    "        <label for=\"input-coord-format\">" << translate("Display position format") << "</label>\n"
    "        <select id=\"input-coord-format\" name=\"coordFormat\" ";
  response.content << ">\n";
  for (const auto &georef_format : georef_formats) {
    response.content
      <<
      "          <option value=\"" << x(georef_format.first) << "\"";
    append_element_selected_flag(response.content, georef_format.first == coord_format);
    response.content
      <<
      ">" << x(georef_format.second) << "</option>\n";
  }
  response.content
    <<
    "        </select>\n"
    "      </div>\n"
    // "      <div ng-show=\"coordFormat !== 'plus+code' && coordFormat !== 'osgb36' && coordFormat !== 'IrishGrid' && coordFormat !== 'ITM'\">\n"
    "      <div id=\"position-style-div\" class=\"col-sm-6 col-lg-4\">\n"
    // Label for selecting the formatting style for a location
    "        <label for=\"position-separator\">" << translate("Position ordering and separator") << "</label>\n"
    "        <select id=\"position-separator\" name=\"positionFormat\"";
  response.content <<
    ">\n"
    "          <option value=\"lat-lng\"";
  append_element_selected_flag(response.content, "lat-lng" == position_format);
  response.content <<
    ">lat lng</option>\n"
    "          <option value=\"lat,lng\"";
  append_element_selected_flag(response.content, "lat,lng" == position_format);
  response.content <<
    ">lat,lng</option>\n"
    "          <option value=\"lng-lat\"";
  append_element_selected_flag(response.content, "lng-lat" == position_format);
  response.content <<
    ">lng lat (Proj4 reversed)</option>\n"
    "          <option value=\"lng,lat\"";
  append_element_selected_flag(response.content, "lng,lat" == position_format);
  response.content <<
    ">lng,lat (Reversed)</option>\n"
    "        </select>\n"
    "      </div>\n"
    "      <div class=\"col-12\"><p id=\"position-text\"></p></div>\n"
    "      <div class=\"col-sm-6 col-md-8 col-lg-4\">\n"
    // Label for input of an altitude value
    "        <label for=\"input-altitude\">" << translate("Altitude") << "</label>\n"
    "        <input id=\"input-altitude\" name=\"altitude\" type=\"number\" min=\"-9999999\" max=\"99999999\" step=\"any\" value=\"";
  append_value(response.content,
               waypoint->id.first && waypoint->altitude.first,
               waypoint->altitude.second);
  response.content << "\"";
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    ">\n"
    // "        <div ng-show=\"form.$submitted || form.altitude.$touched\">\n"
    // "          <div id=\"range-error-altitude\" class=\"alert alert-warning\" role=\"alert\" ng-show=\"form.altitude.$error.min || form.atitude.$error.max\">Altitude out of range.  Must be between -9,999,999 and 99,999,999</div>\n"
    // "        </div>\n"
    "      </div>\n"
    "      <div class=\"col-md-6 col-lg-4\">\n"
    // Label for selection of GPS symbol value
    "        <label for=\"input-symbol\">" << translate("Symbol") << "</label>\n"
    "        <select id=\"input-symbol\" name=\"wptSymbol\"";
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    ">\n"
    // Select choice entry when no item has been chosen
    "          <option value=\"\">" << translate("-- not set --") << "</option>\n";
  for (const auto &waypoint_symbol : waypoint_symbols) {
    response.content
      <<
      "          <option value=\"" << x(waypoint_symbol.first) << "\"";
    append_element_selected_flag(response.content,
                                 waypoint->symbol.first &&
                                 waypoint_symbol.first == waypoint->symbol.second) ;
    response.content
      <<
      ">" << x(waypoint_symbol.second) << "</option>\n";
  }
  response.content
    <<
    "        </select>\n"
    "      </div>\n"
    "      <div class=\"col-md-6 col-lg-4\">\n"
    // Label for input of a waypoint's time and date
    "        <label for=\"input-time\">" << translate("Time") << "</label>\n"
    "        <input id=\"input-time\" type=\"datetime-local\" name=\"time\" size=\"25\" step=\"1\" value=\"";
  if (!waypoint->id.first && !waypoint->time.first)
    waypoint->time = std::make_pair(true, std::chrono::system_clock::now());
  if (waypoint->time.first)
    response.content << dao_helper::datetime_as_html_input_value(waypoint->time.second);
  response.content << "\"";
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    ">\n"
    "      </div>\n"
    "      <div class=\"col-12\">\n"
    "        <p>The description is not intended to be displayed on GPS devices.</p>\n"
    // Label for input of a waypoint's description
    "        <label for=\"input-description\" style=\"vertical-align:top;\">" << translate("Description") << "</label>\n"
    "        <input id=\"input-description\" name=\"description\" type=\"text\" size=\"30\" value=\"";
  append_value(response.content,
               waypoint->id.first && waypoint->description.first,
               x(waypoint->description.second));
  response.content << "\"";
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    ">\n"
    "      </div>\n"
    "      <div class=\"col-12\">\n"
    "        <p>The comment is intended to be displayed on the GPS device.</p>\n"
    // Label for input of a comment for a waypoint
    "        <label for=\"input-comment\">" << translate("Comment") << "</label>\n"
    "        <textarea id=\"input-comment\" style=\"width: 100%;\" name=\"comment\" rows=\"8\"";
  append_element_disabled_flag(response.content, read_only);
  response.content << ">";
  append_value(response.content,
               waypoint->id.first && waypoint->comment.first,
               x(waypoint->comment.second));
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    "</textarea>\n"
    "      </div>\n"
    "      <div class=\"col-md-8 col-lg-4\">\n"
    "        <label for=\"input-samples\">Garmin averaging sample count</label>\n"
    "        <input id=\"input-samples\" name=\"samples\" type=\"number\" min=\"1\" max=\"99999\" value=\"";
  append_value(response.content,
               waypoint->id.first && waypoint->avg_samples.first,
               waypoint->avg_samples.second);
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    "\">\n"
    "      </div>\n"
    "      <div class=\"col-md-6 col-lg-4\">\n"
    "        <label for=\"input-type\">OsmAnd category (type):</label>\n"
    "        <input id=\"input-type\" name=\"type\" type=\"text\" value=\"";
  append_value(response.content,
               waypoint->id.first && waypoint->type.first,
               x(waypoint->type.second));
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    "\">\n"
    "      </div>\n"
    "      <div class=\"col-md-6 col-lg-4\">\n"
    "        <label for=\"input-color\">OsmAnd colour:</label>\n"
    "        <input id=\"input-color\" name=\"color\" type=\"text\" placeholder=\"#001122\" pattern=\"^#[0-9a-fA-F]{1,6}$\" value=\"";
  append_value(response.content,
               waypoint->id.first && waypoint->color.first,
               x(waypoint->color.second));
  append_element_disabled_flag(response.content, read_only);
  response.content
    <<
    "\">\n"
    // "        <div ng-show=\"form.$submitted || form.color.$touched\">\n"
    // "          <div id=\"error-color\" class=\"alert alert-warning\" role=\"alert\" ng-show=\"form.color.$error.pattern\">Color must be a hexadecimal number preceeding by the # sign, e.g. #b4123adf</div>\n"
    // "        </div>\n"
    "      </div>\n"
    "      <div id=\"wpt-buttons\" class=\"col-12 pt-3\" aria-label=\"Form buttons\">\n";
  if (!read_only) {
    response.content
      <<
      // Label of button to save an itinerary waypoint
      "        <button id=\"btn-save\" class=\"btn btn-lg btn-success\" type=\"submit\" accesskey=\"s\" name=\"action\" value=\"save\">" << translate("Save") << "</button>\n"
      // Confirmation dialog when canceling creating or editing an itinerary waypoint
      "        <button id=\"btn-cancel\" class=\"btn btn-lg btn-danger\" type=\"submit\" accesskey=\"c\" name=\"action\" value=\"cancel\" onclick=\"return confirm('" << translate("Cancel?") << "');\" formnovalidate>"
      // Label of button to cancel changes to an itinerary waypoint
      << translate("Cancel") << "</button>\n"
      // Confirmation dialog when resetting changes to an itinerary waypoint
      "        <button id=\"btn-reset\" type=\"reset\" class=\"btn btn-lg btn-danger\" accesskey=\"r\" onclick=\"return confirm('" << translate("Reset changes?") << "');\">Reset</button>\n";
  } else {
    response.content
      <<
      "        <button id=\"btn-close\" class=\"btn btn-lg btn-danger\" type=\"submit\" accesskey=\"c\" name=\"action\" value=\"cancel\">"
      // Label of button to close a read-only view of itinerary waypoint details
      << translate("Close") << "</button>\n";
  }
  response.content
    <<
    "      </div>\n"
    "    </div>\n"
    "  </form>\n"
    "</div>\n";
}

void ItineraryWaypointEditHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  const std::string action = request.get_post_param("action");
  // std::cout << "Action: \"" << action << "\"\n";
  // auto pp = request.get_post_params();
  // for (auto const &p : pp) {
  //   std::cout << "post param: \"" << p.first << "\" -> \"" << p.second << "\"\n";
  // }
  if (action == "cancel") {
    redirect(request, response,
             get_uri_prefix() + "/itinerary?id=" +
             std::to_string(itinerary_id) + "&active-tab=features");
    return;
  }
  if (request.method == HTTPMethod::post) {
    // Save the selected position formatting to the user's session
    coord_format = request.get_post_param("coordFormat");
    position_format = request.get_post_param("positionFormat");
    if (!coord_format.empty() && !position_format.empty()) {
      SessionPgDao session_dao;
      json j;
      j["coord_format"] = coord_format;
      j["position_format"] = position_format;
      session_dao.save_value(get_session_id(),
                             SessionPgDao::coordinate_format_key,
                             j.dump());
    }
  } else if (request.method == HTTPMethod::get) {
    SessionPgDao session_dao;
    std::string json_str =
      session_dao.get_value(get_session_id(),
                            SessionPgDao::coordinate_format_key);
    if (!json_str.empty()) {
      json j = json::parse(json_str);
      coord_format = j["coord_format"].get<std::string>();
      position_format = j["position_format"].get<std::string>();
    }
  }
  ItineraryPgDao dao;
  if (action == "save") {
    ItineraryPgDao::waypoint wpt;
    wpt.id = request.get_optional_post_param_long("waypoint_id");
    wpt.name = request.get_optional_post_param("name");
    if (wpt.name.first)
      dao_helper::trim(wpt.name.second);
    wpt.longitude = std::stod(request.get_post_param("lng"));
    wpt.latitude = std::stod(request.get_post_param("lat"));
    const auto time_str = request.get_optional_post_param("time");
    wpt.time.first = time_str.first && !time_str.second.empty();
    if (wpt.time.first)
      wpt.time.second = DateTime(time_str.second).time_tp();
    wpt.altitude = request.get_optional_post_param_double("altitude");
    wpt.symbol = request.get_optional_post_param("wptSymbol");
    // Don't trim comment as trailing whitespace can be deliberate and necessary
    wpt.comment = request.get_optional_post_param("comment");
    wpt.description = request.get_optional_post_param("description");
    if (wpt.description.first)
      dao_helper::trim(wpt.description.second);
    wpt.color = request.get_optional_post_param("color");
    if (wpt.color.first)
      dao_helper::trim(wpt.color.second);
    wpt.type = request.get_optional_post_param("type");
    if (wpt.type.first)
      dao_helper::trim(wpt.type.second);
    wpt.avg_samples = request.get_optional_post_param_long("samples");
#ifdef HAVE_GDAL
  if (elevation_service) {
    auto altitude = elevation_service->get_elevation(
        wpt.longitude,
        wpt.latitude);
    if (!wpt.altitude.first)
      wpt.altitude = altitude;
  }
#endif
    dao.save(get_user_id(), itinerary_id, wpt);
    redirect(request, response,
             get_uri_prefix() + "/itinerary?id=" +
             std::to_string(itinerary_id) + "&active-tab=features");
    return;
  }
  auto itinerary = dao.get_itinerary_summary(get_user_id(), itinerary_id);
  if (!itinerary.first)
    throw BadRequestException("Itinerary ID not found");
  read_only = itinerary.second.shared_to_nickname.first;
  auto georef_formats = dao.get_georef_formats();
  auto waypoint_symbols = dao.get_waypoint_symbols();
  ItineraryPgDao::waypoint waypoint;
  if (waypoint_id.first)
    waypoint = dao.get_waypoint(get_user_id(), itinerary_id, waypoint_id.second);
  build_form(request, response, &waypoint, georef_formats, waypoint_symbols);
}
