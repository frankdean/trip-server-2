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
#include "itinerary_path_name_edit.hpp"
#include "itinerary_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <algorithm>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

void ItineraryPathNameEdit::build_form(std::ostream &os)
{
  os <<
    "<h1>" << get_page_title() << "</h1>\n"
    "<div class=\"container-fluid\">\n"
    "  <form name=\"form\" method=\"post\">\n"
    "    <div class=\"container-fluid bg-light row g-3 my-3 pb-3 mx-0\">\n"
    "      <div>\n"
    "        <input type=\"hidden\" name=\"itinerary_id\" value=\"" << itinerary_id << "\">\n"
    "        <input type=\"hidden\" name=\"path_id\" value=\"" << path_id << "\">\n"
    // The label for the name field of a route or track
    "        <label for=\"input-name\">" << translate("Name") << "</label>\n"
    "        <input id=\"input-name\" size=\"60\" name=\"name\" value=\"" << (name.first ? x(name.second) : "") << "\" autofocus>\n";
  if (distance.first) {
    os <<
      // Shows the total distance for a route or track in kilometers, when editing the name
      "        <span>&nbsp;" << format(translate("{1,num=fixed,precision=2}&nbsp;km")) % distance.second
      // Shows the total distance for a route or track in miles, when editing the name
       << "&nbsp;" << format(translate("{1,num=fixed,precision=2}&nbsp;mi")) % (distance.second / 1.609344) << "</span>\n";
  }
  os <<
    "      </div>\n"
    "      <div>\n"
    // Label for HTML select dropdown to pick a color for a route or track
    "        <label for=\"input-color\">" << translate("Color") << "</label>\n"
    "        <select id=\"input-color\" name=\"color_key\">\n"
    // Option displayed in an HTML select dropdown to indicate that no color is selected
    "          <option value=\"\">-- not set --</option>\n";
  for (const auto &c : colors) {
    os << "          <option value=\"" << x(c.first) << "\"";
    append_element_selected_flag(os, color_key.first && color_key.second == c.first);
    os << ">" << x(c.second) << "</option>\n";
  }
  os <<
    "        </select>\n"
    "      </div>\n";
  insert_extra_form_controls(os);
  os <<
    "      <div id=\"wpt-buttons\" class=\"col-12 pt-3\" aria-label=\"Form buttons\">\n"
    // Label of button to save an edited route or track name
    "        <button id=\"btn-save\" class=\"btn btn-lg btn-success\" name=\"action\" value=\"save\" accesskey=\"s\">" << translate("Save") << "</button>\n"
    "        <button id=\"btn-cancel\" class=\"btn btn-lg btn-danger\" accesskey=\"c\" name=\"action\" value=\"cancel\" onclick=\"return confirm('" << translate("Cancel?") << "');\" formnovalidate>"
    // Label of button to cancel changes when editing a route or track name
      << translate("Cancel") << "</button>\n"
    // Confirmation dialog when resetting changes when editing a route or track name
    "          <button id=\"btn-reset\" type=\"reset\" class=\"btn btn-lg btn-danger\" accesskey=\"r\" onclick=\"return confirm('" << translate("Reset changes?") << "');\">Reset</button>\n"
    "      </div>\n"
    "    </div>\n"
    "  </form>\n"
    "</div>\n";
}

void  ItineraryRouteNameEdit::insert_extra_form_controls(std::ostream &os)
{
  os <<
    "      <div>\n"
    // Label when editing a route or track name, for a checkbox indicating that
    // a copy of the route should be made instead of changing the current route
    // or track
    "        <label for=\"input-copy\">" << translate("Save as copy") << "</label>\n"
    "        <input id=\"input-copy\" type=\"checkbox\" name=\"make_copy\">\n"
    // Label when editing a route or track name, for a checkbox indicating that a
    // reversed copy of the route should be made
    "        <label for=\"input-reverse\" style=\"padding-left: 1em\">Reverse route</label>\n"
    "        <input id=\"input-reverse\" name=\"reverse_route\" type=\"checkbox\">\n"
    "      </div>\n";
}

void ItineraryRouteNameEdit::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  set_page_title(translate("Itinerary Route"));
  set_menu_item(unknown);
}
  
void ItineraryRouteNameEdit::load_path_data(
    const HTTPServerRequest &request, ItineraryPgDao &dao)
{
  auto route = dao.get_route_summary(get_user_id(), itinerary_id, path_id);
  if (!route.id.first)
    throw BadRequestException("Route ID not set");
  path_id = route.id.second;
  name = route.name;
  color_key = route.color;
  distance = route.distance;
}

void ItineraryRouteNameEdit::save_path(ItineraryPgDao &dao)
{
  if (reverse_route || make_copy) {
    auto route = dao.get_route(get_user_id(), itinerary_id, path_id);
    route.name = name;
    route.color = color_key;
    if (reverse_route) {
      std::reverse(route.points.begin(), route.points.end());
      route.calculate_statistics();
    }
    if (make_copy) {
      route.id.first = false;
      dao.create_route(get_user_id(), itinerary_id, route);
    } else {
      dao.save(get_user_id(), itinerary_id, route);
    }
  } else {
    ItineraryPgDao::route route;
    route.id.first = true;
    route.id.second = path_id;
    route.name = name;
    route.color = color_key;
    dao.update_route_summary(get_user_id(), itinerary_id, route);
  }
}

void ItineraryTrackNameEdit::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  set_page_title(translate("Itinerary Track"));
  set_menu_item(unknown);
}
  
void ItineraryTrackNameEdit::load_path_data(
    const HTTPServerRequest &request, ItineraryPgDao &dao)
{
  auto track = dao.get_track_summary(get_user_id(), itinerary_id, path_id);
  if (!track.id.first)
    throw BadRequestException("Track ID not set");
  path_id = track.id.second;
  name = track.name;
  color_key = track.color;
  distance = track.distance;
}

void ItineraryTrackNameEdit::save_path(ItineraryPgDao &dao)
{
  ItineraryPgDao::track track;
  track.id.first = true;
  track.id.second = path_id;
  track.name = name;
  track.color = color_key;
  dao.update_track_summary(get_user_id(), itinerary_id, track);
}

void ItineraryPathNameEdit::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  ItineraryPgDao dao;
  itinerary_id = std::stol(request.get_param("itinerary_id"));
  path_id = std::stol(request.get_param("path_id"));
  if (request.method == HTTPMethod::post) {
    name.second = request.get_post_param("name");
    dao_helper::trim(name.second);
    name.first = !name.second.empty();
    color_key.second = request.get_post_param("color_key");
    color_key.first = !color_key.second.empty();
    make_copy = request.get_post_param("make_copy") == "on";
    reverse_route = request.get_post_param("reverse_route") == "on";
    const std::string action = request.get_param("action");
    if (action == "save")
      save_path(dao);
    redirect(request, response, get_uri_prefix() + "/itinerary?id=" +
             std::to_string(itinerary_id) + "&active-tab=features");
    return;
  }
  load_path_data(request, dao);
  colors = dao.get_path_color_options();
  build_form(response.content);
}
