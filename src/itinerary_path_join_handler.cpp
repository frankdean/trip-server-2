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
#include "itinerary_path_join_handler.hpp"
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

void ItineraryPathJoinHandler::build_form(
    std::ostream &os,
    const std::vector<ItineraryPgDao::path_summary> paths)
{
  const auto first_path = paths.front();
  if (!joined_path_color_key.has_value())
    joined_path_color_key = first_path.color_key;
  os <<
    "<div class=\"container-fluid\">\n"
    "  <h1>" << get_page_title() << "</h1>\n"
    "  <form name=\"form\" method=\"post\">\n"
    "    <input type=\"hidden\" name=\"itineraryId\" value=\"" << itinerary_id << "\">\n"
    "    <table id=\"paths-table\" class=\"table table-striped\">\n";

  int position = 0;
  for (const auto &path : paths) {
    if (!joined_path_name && path.name) {
      joined_path_name = path.name;
      append_name(joined_path_name, "(joined)");
    }
    if (path.id.has_value()) {
      os <<
        "      <tr>\n"
        "        <td>\n"
        "        <input type=\"hidden\" name=\"path_id[" << position << "]\" value=\"" << path.id.value() << "\">\n"
        // Label for moving a track or path up in a list of paths being joined
        "          <button class=\"btn btn-info mb-2\" name=\"up[" << position << "]\" value=\"" << path.id.value() << "\" accesskey=\"u\">" << translate("Up") << "</button>\n"
        // Label for moving a track or path down in a list of paths being joined
        "          <button class=\"btn btn-info mb-2\" name=\"down[" << position << "]\" value=\"" << path.id.value() << "\" accesskey=\"d\">" << translate("Down") << "</button>\n"
        "        </td>\n"
        "        <td><span>";
      if (path.name.has_value()) {
        os << x(path.name.value());
      } else if (path.id.has_value()) {
        // Database ID of an item, typically a path, track or waypoint
        os << format(translate("ID:&nbsp;{1,number=left}")) % path.id.value();
      }
      os <<
        "</span></td>\n"
        "        <td>" << (path.color_description.has_value() ? x(path.color_description.value()) : "") << "</td>\n"
        "        <td>";
      if (path.distance.has_value()) {
        os <<
          // Shows distance in kilometers
          format(translate("{1,num=fixed,precision=2}&nbsp;km")) % path.distance.value();
      }
    }
    os
      <<
      "<td>\n"
      "        <td>";
    if (path.distance.has_value()) {
      os <<
        // Shows distance in miles
        format(translate("{1,num=fixed,precision=2}&nbsp;mi")) % (path.distance.value() / kms_per_mile);
    }
    os << "<td>\n";
    if (path.ascent.has_value() || path.descent.has_value()) {
      const double ascent = path.ascent.has_value() ? path.ascent.value() : 0;
      const double descent = path.descent.has_value() ? path.descent.value() : 0;
      os
        <<
        "        <td>"
        // Shows the total ascent and descent of a path in meters
        << format(translate("&#8599;{1,num=fixed,precision=0}&nbsp;m &#8600;{2,num=fixed,precision=0}&nbsp;m")) % ascent % descent
        << "</td>\n"
        "        <td>"
        // Shows the total ascent and descent of a path in ft
        << format(translate("&#8599;{1,num=fixed,precision=0}&nbsp;ft &#8600;{2,num=fixed,precision=0}&nbsp;ft"))
        % (ascent / feet_per_meter) % (descent / feet_per_meter)
        << "</td>\n";
    } else {
      os << "        <td></td><td></td>\n";
    }
    if (path.highest.has_value() || path.lowest.has_value()) {
      const double highest = path.highest.has_value() ? path.highest.value() : 0;
      const double lowest = path.lowest.has_value() ? path.lowest.value() : 0;
      os
        <<
        "        <td>"
        // Shows the total highest and lowest points of a path in meters
        << format(translate("{1,num=fixed,precision=0}&#x21c5;{2,num=fixed,precision=0}&nbsp;m")) % highest % lowest
        << "</td>\n"
        "        <td>"
        // Shows the total highest and lowest points of a path in ft
        << format(translate("{1,num=fixed,precision=0}&#x21c5;{2,num=fixed,precision=0}&nbsp;ft"))
        % (highest / feet_per_meter) % (lowest / feet_per_meter)
        << "</td>\n";
    } else {
      os << "        <td></td><td></td>\n";
    }
    os
      <<
      "        </tr>\n";
    position++;
  }
  os <<
    "    </table>\n"
    "    <div>\n"
    // label prompting for input of a new name for a joined path
    "      <label for=\"input-name\" accesskey=\"n\">" << translate("New path name") << "</label>\n"
    "      <input id=\"input-name\" name=\"name\" value=\"" << x(joined_path_name) << "\" size=\"30\">\n"
    "    </div>\n"
    "    <div class=\"mb-3\">\n"
    // Label for selecting a color for a joined path
    "      <label for=\"input-color\">" << translate("New path color") << "</label>\n"
    "      <select id=\"input-color\" name=\"color_key\" accesskey=\"l\">\n"
    // Option displayed in an HTML select dropdown to indicate that no color is selected
    "        <option value=\"\">-- not set --</option>\n";
  for (const auto &c : colors) {
    os << "        <option value=\"" << x(c.first) << "\"";
    append_element_selected_flag(os, joined_path_color_key.has_value() && joined_path_color_key.value() == c.first);
    os << ">" << x(c.second) << "</option>\n";
  }
  os <<
    "      </select>\n"
    "    </div>\n"
    // Label of button to join a set of paths
    "    <button id=\"btn-join\" class=\"btn btn-lg btn-success\" name=\"action\" value=\"join\" accesskey=\"j\">" << translate("Join") << "</button>\n"
    // Label to cancel joining a set of paths
    "    <button id=\"btn-cancel\" class=\"btn btn-lg btn-danger\" name=\"action\" value=\"cancel\" accesskey=\"c\">" << translate("Cancel") << "</button>\n"
    "  </form>\n"
    "</div>\n";
  if (!path_ids.empty()) {
    os <<
      "<div id=\"itinerary-path-map\"></div>\n";
    os <<
      "<script>\n"
      "<!--\n";
    nlohmann::json features = get_selected_features();
    json j{
      {"itinerary_id", itinerary_id},
      {"features", features}
    };
    // std::cout << j.dump(4) << '\n';
    os <<
      "const pageInfoJSON = '" << j << "';\n"
      "const server_prefix = '" << get_uri_prefix() << "';\n";
    append_map_provider_configuration(os);
    os <<
      "// -->\n"
      "</script>\n";
  }
}

void ItineraryPathJoinHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  itinerary_id = std::stol(request.get_param("itineraryId"));
  const std::vector<std::string> str_path_ids =
    UriUtils::split_params(request.get_query_param("path-ids"), ",");
  for (const auto &s : str_path_ids)
    path_ids.push_back(std::stol(s));
  set_menu_item(unknown);
  if (request.method == HTTPMethod::post) {
    action = request.get_param("action");
    // std::cout << "Action: \"" << action << "\"\n";
    joined_path_name = request.get_optional_post_param("name");
    const auto color_key = request.get_param("color_key");
    if (!color_key.empty())
      joined_path_color_key = color_key;
    down_action_map = request.extract_array_param_map("down");
    up_action_map = request.extract_array_param_map("up");
    posted_paths_map = request.extract_array_param_map("path_id");
    // for (const auto &i : posted_paths_map) {
    //   std::cout << "Post path \"" << i.first << "\" -> \"" << i.second << "\"  up\n";
    // }
  }
}

void ItineraryPathJoinHandler::append_pre_body_end(std::ostream& os) const
{
  BaseMapHandler::append_pre_body_end(os);
  os << "    <script type=\"module\" src=\"" << get_uri_prefix() << "/static/js/itinerary-path-join.js\"></script>\n";
}

void ItineraryPathJoinHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  ItineraryPgDao dao;
  if (action != "cancel" && !path_ids.empty()) {
    colors = dao.get_path_color_options();
    auto db_paths = get_paths(dao);
    if (!posted_paths_map.empty()) {
      // Re-order the paths read from the database to match the current order of this post
      // std::cout << "Re-ordering paths read from DB\n";
      std::map<long, ItineraryPgDao::path_summary> temp_map;
      for (const auto &p : db_paths)
        if (p.id.has_value())
          temp_map[p.id.value()] = p;
      for (const auto &p : posted_paths_map)
        paths.push_back(temp_map.at(std::stol(p.second)));
      // for (const auto &path : paths) {
      //   if (path.id.first)
      //     std::cout << "Path from DB after re-ordering: " << path.id.second << '\n';
      // }
    } else {
      paths = db_paths;
    }
  }
  if (action == "join") {
    std::vector<long> ids;
    for (const auto &p : posted_paths_map)
      ids.push_back(std::stol(p.second));
    join_paths(dao, ids);
  } else if (!up_action_map.empty()) {
    for (const auto &i : up_action_map) {
      // std::cout << "Moving \"" << i.first << "\" -> \"" << i.second << "\"  up\n";
      const int target = i.first -1;
      if (target < 0) {
        std::rotate(paths.begin(), paths.begin() + 1, paths.end());
      } else {
        // std::cout << "Swapping position \"" << i.first << "\" to position \"" << target << "\"\n";
        std::swap(paths[i.first], paths[target]);
        // for (const auto &path : paths) {
        //   if (path.id.first)
        //     std::cout << "Path: " << path.id.second << '\n';
        // }
      }
    }
  } else if (!down_action_map.empty()) {
    for (const auto &i : down_action_map) {
      // std::cout << "Moving \"" << i.first << "\" -> \"" << i.second << "\"  down\n";
      const int target = i.first +1;
      if (target >= paths.size()) {
        std::rotate(paths.rbegin(), paths.rbegin() + 1, paths.rend());
      } else {
        // std::cout << "Swapping position \"" << i.first << "\" to position \"" << target << "\"\n";
        std::swap(paths[i.first], paths[target]);
        // for (const auto &path : paths) {
        //   if (path.id.first)
        //     std::cout << "Path: " << path.id.second << '\n';
        // }
      }
    }
  }
  if (action != "join" && action != "cancel" && !path_ids.empty()) {
    build_form(response.content, paths);
    return;
  }
  redirect(request, response, get_uri_prefix() + "/itinerary?id=" +
           std::to_string(itinerary_id) + "&active-tab=features");
}
