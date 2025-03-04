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
#include "itinerary_route_edit_handler.hpp"
#include "geo_utils.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/pagination.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>
// #include <algorithm>
// #include <sstream>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

void ItineraryRouteEditHandler::build_form(std::ostream &os,
                                           const web::Pagination& pagination,
                                           const ItineraryPgDao::route &route)
{
  os <<
    "<div class=\"container-fluid\">\n"
    //
    "  <h1>" << get_page_title() << "</h1>\n"
    "  <form name=\"form\" method=\"post\">\n"
    "    <input type=\"hidden\" name=\"id\" value=\"" << itinerary_id << "\">\n"
    "    <input type=\"hidden\" name=\"itineraryId\" value=\"" << itinerary_id << "\">\n"
    "    <input type=\"hidden\" name=\"routeId\" value=\"" << route.id.value() << "\">\n"
    "    <input type=\"hidden\" name=\"shared\" value=\"" << (read_only ? "true" : "false") << "\">\n"
    "    <input type=\"hidden\" name=\"active-tab\" value=\"features\">\n";
  if (route.points.empty()) {
    os <<
      "    <div id=\"route-not-found\" class=\"alert alert-info\">\n"
      // Information alert shown when a route has no points
      "      <p>" << translate("Route has no points") << "</p>\n"
      "    </div>\n";
  } else {
    os <<
      "    <div>";
    if (route.name.has_value()) {
      // Formatted output of label and route name
      os << format(translate("<strong>Name:</strong>&nbsp;{1}")) % x(route.name.value());
    } else if (route.id.has_value()) {
      // Database ID of an item, typically a route, track or waypoint
      os << format(translate("<strong>ID:</strong>&nbsp;{1,number=left}")) % route.id.value();
    }
    os << "</div>\n";
    if (route.color_description.has_value()) {
      // Formatted output of label and route color
      os << "    <div>" << format(translate("<strong>Color:</strong>&nbsp;{1}")) % x(route.color_description.value()) << "</div>\n";
    }
    os << "    <div class=\"mb-3\">\n";
    if (route.distance.has_value()) {
      // Shows the total distance for a route or route in kilometers
      os
        << "      <span>&nbsp;" << format(translate("{1,num=fixed,precision=2}&nbsp;km")) % route.distance.value()
        // Shows the total distance for a route or route in miles
        << "&nbsp;" << format(translate("{1,num=fixed,precision=2}&nbsp;mi")) % (route.distance.value() / kms_per_mile) << "</span>\n";
    }
    if (route.ascent.has_value()) {
      // Shows the total ascent for a route or route in meters
      os << "      <span>&nbsp;↗︎" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % route.ascent.value() << "</span>\n";
    }
    if (route.descent.has_value()) {
      // Shows the total descent for a route or route in kilometers
      os << "      <span>&nbsp;↘" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % route.descent.value() << "</span>\n";
    }
    if (route.ascent.has_value()) {
      // Shows the total ascent for a route or route in feet
      os << "      <span>&nbsp;↗︎" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (route.ascent.value() / inches_per_meter / 12) << "</span>\n";
    }
    if (route.descent.has_value()) {
      // Shows the total descent for a route or route in feet
      os << "      <span>&nbsp;↘" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (route.descent.value() / inches_per_meter / 12) << "</span>\n";
    }
    if (route.highest.has_value()) {
      os
        // Shows the highest point of a route or route in meters
        << "      <span>&nbsp;" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % route.highest.value() << "</span>";
    }
    if (route.highest.has_value() || route.lowest.has_value())
      os << "&nbsp;⇅";
    if (route.lowest.has_value()) {
      os
        // Shows the lowest point of a route or route in meters
        << "<span>" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % route.lowest.value() << "</span>\n";
    }
    if (route.highest.has_value()) {
      os
        // Shows the highest point of a route or route in meters
        << "      <span>&nbsp;" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (route.highest.value() / inches_per_meter / 12) << "</span>";
    }
    if (route.highest.has_value() || route.lowest.has_value())
      os << "&nbsp;&#x21c5;";
    if (route.lowest.has_value()) {
      os
        // Shows the lowest point of a route or route in meters
        << "<span>" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (route.lowest.value() / inches_per_meter / 12) << "</span>\n";
    }
    os
      <<
      "    </div>\n"
      "      <table id=\"selection-list\" class=\"table table-striped\">\n"
      "        <tr>\n"
      "          <th>\n"
      "            <input id=\"input-select-all\" type=\"checkbox\" name=\"select-all\" accesskey=\"a\"";
    if (select_all)
      os << " checked";
    os <<
      ">\n"
      // Column heading for route point IDs
      "            <label for=\"input-select-all\">" << translate("ID") << "</label>\n"
      "          </th>\n"
      // Column heading for route point latitudes
      "          <th class=\"text-end\">" << translate("Latitude") << "</th>\n"
      // Column heading for route point longitudes
      "          <th class=\"text-end\">" << translate("Longitude") << "</th>\n"
      // Column heading for route point altitudes
      "          <th class=\"text-end\">" << translate("Altitude") << "</th>\n"
      // Column heading for route name
      "          <th class=\"text-start\">" << translate("Name") << "</th>\n"
      // Column heading for route comment
      "          <th class=\"text-start\">" << translate("Comment") << "</th>\n"
      // Column heading for route description
      "          <th class=\"text-start\">" << translate("Description") << "</th>\n"
      // Column heading for route symbol
      "          <th class=\"text-start\">" << translate("Symbol") << "</th>\n"
      "        </tr>\n";
    for (const auto &point : route.points) {
      if (point.id.has_value()) {
        std::ostringstream label;
        label << "select-point-" << point.id.value();
        os <<
          "        <tr>\n"
          "          <td class=\"text-start\">\n            <input id=\"" << label.str() << "\" type=\"checkbox\" name=\"point[" << point.id.value() << "]\" value=\"" << point.id.value() << "\"";
        if (select_all || selected_point_id_map.find(point.id.value()) != selected_point_id_map.end())
          os << " checked";
        os << ">\n"
          "            <label for=\"" << label.str() << "\">" << as::number << std::setprecision(0) << point.id.value() << as::posix << "</label>\n"
          "          </td>\n"
          "          <td class=\"text-end\">" << std::fixed << std::setprecision(6) << point.latitude << "</td>\n"
          "          <td class=\"text-end\">" << point.longitude << "</td>\n"
          "          <td class=\"text-end\">";
        if (point.altitude.has_value())
          os << std::fixed << std::setprecision(0) << point.altitude.value();
        os <<
          "</td>\n"
          "          <td class=\"text-start\">";
        if (point.name.has_value())
          os << x(point.name.value());
        os << "</td>\n"
          "          <td class=\"text-start\">";
        if (point.comment.has_value())
          os << x(point.comment.value());
        os << "</td>\n"
          "          <td class=\"text-start\">";
        if (point.description.has_value())
          os << x(point.description.value());
        os << "</td>\n"
          "          <td class=\"text-start\">";
        if (point.symbol.has_value())
          os << x(point.symbol.value());
        os << "</td>\n"
          "        </tr>\n";
      }
    }
    os <<
      "      </table>\n";
    const auto page_count = pagination.get_page_count();
    if (page_count > 1) {
      os
        <<
        "    <div id=\"div-paging\" class=\"pb-0\">\n"
        << pagination.get_html()
        <<
        "    </div>\n"
        "    <div class=\"d-flex justify-content-center pt-0 pb-0 col-12\">\n"
        "      <input id=\"page\" type=\"number\" name=\"page\" value=\""
        << std::fixed << std::setprecision(0) << pagination.get_current_page()
        << "\" min=\"1\" max=\"" << page_count << "\">\n"
        // Title of button which goes to a specified page number
        "      <button id=\"goto-page-btn\" class=\"btn btn-sm btn-primary\" type=\"submit\" name=\"action\" accesskey=\"g\" value=\"page\">" << translate("Go") << "</button>\n"
        "    </div>\n"
        ;
    }
  }
  os <<
    "    <div id=\"div-buttons\">\n";
  if (!read_only && !route.points.empty()) {
    os <<
      // Confirmation to delete one or more selected points from a route
      "      <button id=\"btn-delete\" name=\"action\" value=\"delete\" accesskey=\"d\" class=\"my-1 btn btn-lg btn-danger\" onclick=\"return confirm('" << translate("Delete the selected points?") << "');\">"
      // Button label for deleting a seletion of points in a route
       << translate("Delete points") << "</button>\n"
      // Confirmation to split a route by selected point
      "      <button id=\"btn-split\" name=\"action\" value=\"split\" accesskey=\"s\" class=\"my-1 btn btn-lg btn-danger\" onclick=\"return confirm('" << translate("Split route before selected point?") << "');\">"
      // Button label to split a route at a selected point
       << translate("Split route") << "</button>\n"
      "      <button id=\"btn-reverse\" name=\"action\" value=\"reverse\" accesskey=\"r\" class=\"my-1 btn btn-lg btn-warning\">"
      // Button label to reverse a route
       << translate("Reverse") << "</button>\n";
  }
  os <<
    // Label for button to return to the itinerary when viewing a list of route points
    "        <button id=\"btn-close\" accesskey=\"c\" formmethod=\"get\" formaction=\"" << get_uri_prefix() << "/itinerary\" class=\"my-1 btn btn-lg btn-danger\">" << translate("Close") << "</button>\n"
    "    </div>\n"
    "  </form>\n"
    "</div>\n";
  if (!route.points.empty()) {
    os <<
      "<div id=\"itinerary-route-map\"></div>\n";
    json features;
    json points;
    for (const auto &point : route.points) {
      if (point.id.has_value())
        points.push_back(point.id.value());
    }
    json j{
      {"itinerary_id", itinerary_id},
      {"route_id", route_id},
      {"route_point_ids", points}
    };
    // std::cout << "pageInfoJSON:\n" << j.dump(4) << '\n';
    os <<
      "<script>\n"
      "<!--\n"
      "const pageInfoJSON = '" << j << "';\n"
      "const server_prefix = '" << get_uri_prefix() << "';\n";
    append_map_provider_configuration(os);
    os <<
      "// -->\n"
      "</script>\n";
  }
}

void ItineraryRouteEditHandler::delete_points(
    const web::HTTPServerRequest& request,
    ItineraryPgDao &dao)
{
  (void)request; // unused
  bool dirty = false;
  auto route = dao.get_route(get_user_id(), itinerary_id, route_id);
  route.points.erase(
      std::remove_if(
          route.points.begin(),
          route.points.end(),
          [&](const ItineraryPgDao::route_point &point) {
            bool retval = point.id.has_value() &&
              selected_point_id_map.find(point.id.value()) !=
              selected_point_id_map.end();
            if (retval)
              dirty = true;
            return retval;
          }),
      route.points.end());
  if (dirty) {
    route.calculate_statistics();
    dao.save(get_user_id(), itinerary_id, route);
  }
}

void ItineraryRouteEditHandler::split_route(
    const web::HTTPServerRequest& request,
    ItineraryPgDao &dao)
{
  (void)request; // unused
  long split_before_id = selected_point_ids.front();
  auto route = dao.get_route(get_user_id(), itinerary_id, route_id);
  ItineraryPgDao::route new_route(route);
  std::ostringstream os;
  if (route.name.has_value()) {
    // Name given to new section of a split route.  The parameter is the name of
    // the original route.
    os << format(translate("{1} (split)"))
      % route.name.value();
  } else {
    // Name given to new section of a split route which has no name.  The
    // parameter is the ID of the original route
    os << format(translate("ID: {1,number=left} (split)"))
      % route.id.value();
  }
  new_route.name = os.str();

  // Remove points from and after the split point
  route.points.erase(
      std::remove_if(
          route.points.begin(),
          route.points.end(),
          [split_before_id](const ItineraryPgDao::route_point &point) {
            return point.id.has_value() && point.id.value() >= split_before_id;
          }),
      route.points.end());
  // Remove points from the new route before the split (copied from original)
  new_route.points.erase(
      std::remove_if(
          new_route.points.begin(),
          new_route.points.end(),
          [split_before_id](const ItineraryPgDao::route_point &point) {
            return point.id.has_value() && point.id.value() < split_before_id;
          }),
      new_route.points.end());
  if (!new_route.points.empty()) {
    route.calculate_statistics();
    new_route.calculate_statistics();
    dao.create_route(get_user_id(), itinerary_id, new_route);
    dao.save(get_user_id(), itinerary_id, route);
  }
}

void ItineraryRouteEditHandler::reverse_route(
    const web::HTTPServerRequest& request,
    ItineraryPgDao &dao)
{
  (void)request; // unused
  auto route = dao.get_route(get_user_id(), itinerary_id, route_id);
  std::reverse(route.points.begin(), route.points.end());
  route.calculate_statistics();
  dao.save(get_user_id(), itinerary_id, route);
}

void ItineraryRouteEditHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  (void)response; // unused
  itinerary_id = std::stol(request.get_param("itineraryId"));
  route_id = std::stol(request.get_param("routeId"));
  const std::string shared = request.get_param("shared");
  read_only = shared == "true";
  const std::string select_all_str = request.get_param("select-all");
  select_all = select_all_str == "on";
  // title of the itinerary route editing page
  set_page_title(translate("Itinerary Route Edit"));
  set_menu_item(unknown);
  if (request.method == HTTPMethod::post) {
    action = request.get_param("action");
    if (!action.empty()) {
      selected_point_id_map = request.extract_array_param_map("point");
      for (const auto &m : selected_point_id_map) {
        if (!m.second.empty())
          selected_point_ids.push_back(m.first);
      }
    }
  }
}

void ItineraryRouteEditHandler::append_pre_body_end(std::ostream& os) const
{
  BaseMapHandler::append_pre_body_end(os);
  os << "    <script type=\"module\" src=\"" << get_uri_prefix() << "/static/js/itinerary-route-edit.js\"></script>\n";
}

void ItineraryRouteEditHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  ItineraryPgDao dao;
  if (action == "delete") {
    delete_points(request, dao);
  } else if (action == "split" && !selected_point_ids.empty()) {
    split_route(request, dao);
    std::ostringstream url;
    url << get_uri_prefix() << "/itinerary?id=" << itinerary_id
        << "&active-tab=features";
    redirect(request, response, url.str());
    return;
  } else if (action == "reverse") {
    reverse_route(request, dao);
  }
  std::map<std::string, std::string> page_param_map;
  page_param_map["itineraryId"] = std::to_string(itinerary_id);
  page_param_map["routeId"] = std::to_string(route_id);
  page_param_map["shared"] = read_only ? "true" : "false";
  page_param_map["select-all"] = select_all ? "on" : "";
  const long total_count = dao.get_route_point_count(get_user_id(),
                                                       itinerary_id,
                                                       route_id);
  Pagination pagination(get_uri_prefix() + "/itinerary/route/edit",
                        page_param_map,
                        total_count);
  const std::string page = request.get_param("page");
  try {
    if (!page.empty())
      pagination.set_current_page(std::stoul(page));
  } catch (const std::logic_error& e) {
    std::cerr << "Error converting string to page number\n";
  }
  const auto route = dao.get_route_points(get_user_id(),
                                            itinerary_id,
                                            route_id,
                                            pagination.get_offset(),
                                            pagination.get_limit());
  build_form(response.content, pagination, route);
}
