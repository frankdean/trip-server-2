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
#include "itinerary_simplify_handler.hpp"
#include "itinerary_handler.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using json = nlohmann::json;

void ItinerarySimplifyHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  (void)response; // unused
  // Page title for the simplify track page
  set_page_title(translate("Simplify Track"));
  itinerary_id = std::stol(request.get_query_param("id"));
}

void ItinerarySimplifyHandler::build_form(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response,
    const ItineraryPgDao::selected_feature_ids &features)
{
  (void)request; // unused
  json j{
    {"itinerary_id", itinerary_id},
    {"features", features}
  };

  response.content
    <<
    "<div id=\"main-content\" class=\"container-fluid\">\n"
    // Page title for the simplify track page
    "  <h1>" << translate("Simplify Track") << "</h1>\n"
    "  <div>\n"
    "    <form name=\"simplify-path-form\">\n"
    "      <div class=\"container-fluid bg-light py-2\">\n"
    "        <input type=\"hidden\" name=\"id\" value=\"" << itinerary_id << "\">\n"
    "        <p id=\"track-name\"><span class=\"view-label\">" << translate("Name") << "</span><span id=\"path-name\" style=\"padding-left: 1em; padding-right: 1em;\"></span></p>\n"
    "        <p>\n"
    "          <span>\n"
    // Label for a tolerance slider control to set a tolerance value when simplifying a track
    "            <label for=\"tolerance\">" << translate("Tolerance") << "</label>\n"
    "            <input id=\"tolerance\" style=\"width: 100%;\" type=\"range\" min=\"0\" max=\"100\" step=\"1\" value=\"0\" >\n"
    "          </span>\n"
    "        </p>\n"
    // Label for how many locations there are in the original track when simplifying it
    "        <p id=\"original-points\">" << translate("Original points") << " <span id=\"original-point-count\"></span></p>\n"
    // Label for how many locations there are in the simplified track while simplifying a track
    "        <p id=\"current-points\">" << translate("Current points") << " <span id=\"current-point-count\"></span></p>\n"
    "        <div class=\"my-2\">\n"
    // Label to save changes when simplifying a track
    "          <button id=\"btn-save\" type=\"button\" class=\"btn btn-success\" name=\"action\" value=\"save\" accesskey=\"s\">" << translate("Save") << "</button>\n"
    "          <button id=\"btn-cancel\" type=\"button\" class=\"btn btn-danger mx-3\" name=\"action\" value=\"cancel\" accesskey=\"c\">"
    // Button label to cancel simplifying a track
    << translate("Cancel") << "</button>\n"
    "        </div>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n"
    "<div id=\"simplify-track-map\"></div>\n"
    "<script>\n"
    "<!--\n"
    "const pageInfoJSON = '" << j << "';\n"
    "const server_prefix = '" << get_uri_prefix() << "';\n";
  append_map_provider_configuration(response.content);
  response.content <<
    "      // -->\n"
    "    </script>\n";
}

void ItinerarySimplifyHandler::append_pre_body_end(std::ostream& os) const
{
  BaseMapHandler::append_pre_body_end(os);
  os << "    <script type=\"module\" src=\"" << get_uri_prefix()
     << "/static/js/simplify-path.js\"></script>\n";
}

void ItinerarySimplifyHandler::handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response)
{
  const std::string action = request.get_param("action");
  // std::cout << "Action: \"" << action << "\"\n";
  auto features = ItineraryHandler::get_selected_feature_ids(request);
  if (features.tracks.empty()) {
    redirect(request, response,
             get_uri_prefix() + "/itinerary?id=" +
             std::to_string(itinerary_id) + "&active-tab=features");
    return;
  }
  // Simplify one track ??  Works if we display everything, maybe even useful in some situations.
  // if (features.tracks.size() > 1)
  //   features.tracks.erase(features.tracks.begin() + 1, features.tracks.end());
  build_form(request, response, features);
}
