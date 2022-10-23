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
#include "track_sharing_handler.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/pagination.hpp"
#include "../trip-server-common/src/uri_utils.hpp"
#include <map>
#include <boost/locale.hpp>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

void TrackSharingHandler::do_preview_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  // Title for the track sharing page
  set_page_title(translate("Track Sharing"));
  set_menu_item(track_sharing);
}

void TrackSharingHandler::build_form(
    HTTPServerResponse& response,
    const Pagination& pagination,
    const std::vector<TrackPgDao::track_share>& track_shares) const
{
  response.content
    <<
    "<div class=\"container-fluid\">\n"
    // The title of the track sharing page
    "  <h1 class=\"pt-2\">" << translate("Track Sharing") << "</h1>\n"
    "  <form name=\"form\" method=\"post\">\n";
  if (track_shares.empty()) {
    response.content
      << "<p>"
      // Message displayed when there are no track sharing records to show
      << translate("You are not currently sharing your location with anyone else.")
      << "</p>\n";
  } else {
    response.content
      <<
      "  <div id=\"shares\" class=\"table-responsive\">\n"
      "    <table id=\"table-shares\" class=\"table table-striped\">\n"
      "      <tr>\n"
      // The column heading for the nickname of the user tracks may be shared with
      "        <th>" << translate("Nickname") << "</th>\n"
      // The column heading for the recent time period tracks may be shared with
      "        <th>" << translate("Recent Limit") << "</th>\n"
      // The column heading for the maximum time period tracks may be shared with
      "        <th>" << translate("Maximum Limit") << "</th>\n"
      // The column heading showing whether track sharing is active with another user
      "        <th>" << translate("Active") << "</th>\n"
      "        <th><input id=\"select-all\" type=\"checkbox\" class=\"form-check-input\" onclick=\"select_all(this)\"></th>\n"
      "      </tr>\n";
    int c = 0;
    for (auto const &share : track_shares) {
      response.content
        <<
        "      <tr>\n"
        "        <td>" << x(share.nickname) << "</td>\n"
        "        <td>";
      if (share.recent_minutes.first) {
        period_dhm t(share.recent_minutes.second);
        // Abbreviated expression for days, hours and minutes
        response.content << format(translate("{1}d {2}h {3}m"))
          % t.days % t.hours % t.minutes;
      }
      response.content
        << "</td>\n"
        "        <td>";
      if (share.max_minutes.first) {
        period_dhm t(share.max_minutes.second);
        // Abbreviated expression for days, hours and minutes
        response.content << format(translate("{1}d {2}h {3}m"))
          % t.days % t.hours % t.minutes;
      }
      response.content
        << "</td>\n"
        "        <td>" << (share.active.first && share.active.second ? "&check;" : "") << "</td>\n"
        "        <td><input type=\"checkbox\" name=\"nickname[" << c << "]\" value=\"" << x(share.nickname) << "\"></td>\n"
        "      </tr>\n";
      c++;
    } // for
    response.content
      <<
      "    </table>\n"
      "  </div>\n";
    if (pagination.get_page_count() > 0) {
      response.content
        <<
        "  <div id=\"div-paging\" class=\"pb-0\">\n"
        << pagination.get_html()
        <<
        "  </div>\n";
    }
  } // if (!track_shares.empty())

  response.content
    <<
    "      <div id=\"div-buttons\" class=\"py-1\">\n";
  if (!track_shares.empty()) {
    response.content
      <<
      // Name of button to activate track sharing with selected users
      "        <button id=\"btn-activate\" name=\"action\" value=\"activate\" class=\"my-1 btn btn-lg btn-success\">" << translate("Activate") << "</button>\n"
      // Name of button to de-activate track sharing with selected users
      "        <button id=\"btn-deactivate\" name=\"action\" value=\"de-activate\" class=\"my-1 btn btn-lg btn-primary\">" << translate("Deactivate") << "</button>\n"
      // Prompt to confirm the user wishes to delete track sharing for the selected users.
      "        <button id=\"btn-delete\" name=\"action\" value=\"delete\" class=\"my-1 btn btn-lg btn-danger\" onclick=\"return confirm('" << translate("Delete the selected location shares?") << "');\">"
      // Name of button to delete track sharing with selected users
      << translate("Delete selected") << "</button>\n"
      // Name of button to edit the track sharing criteria of a selected user
      "        <button id=\"btn-edit\" name=\"action\" value=\"edit\" class=\"my-1 btn btn-lg btn-info\">" << translate("Edit selected") << "</button>\n";
  }
  response.content
    <<
    // Name of button to create a new track sharing record
    "    <button id=\"btn-new\" formaction=\"" << get_uri_prefix() << "/sharing/edit?new=true\" class=\"my-1 btn btn-lg btn-warning\">" << translate("New") << "</button>\n"
    "  </form>\n"
    "</div>\n"
    "<script type=\"text/javascript\">\n"
    "<!--\n"
    "function select_all(cb) {\n"
    "  const div = document.getElementById('shares');\n"
    "  const all = div.getElementsByTagName('input');\n"
    "  for (let i = 0; i < all.length; i++) {\n"
    "    if (all[i] !== cb && all[i].type == 'checkbox') {\n"
    "      all[i].checked = cb.checked;\n"
    "    }\n"
    "  }\n"
    "}\n"
    "// -->\n"
    "</script>\n";
}


void TrackSharingHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  const int max_page_size = 10;
  TrackPgDao dao;
  if (request.method == HTTPMethod::post) {
    const std::string action = request.get_post_param("action");
    // std::cout << "Action: \"" << action << "\"\n";
    // auto pp = request.get_post_params();
    // for (auto const &p : pp) {
    //   std::cout << "param: \"" << p.first << "\" -> \"" << p.second << "\"\n";
    // }
    std::vector<std::string> nicknames;
    for (int i = 0; i < max_page_size; ++i) {
      const std::string nickname =
        request.get_post_param("nickname[" + std::to_string(i) + "]");
      if (!nickname.empty())
        nicknames.push_back(nickname);
    }
    if (!nicknames.empty()) {
      if (action == "delete") {
        dao.delete_track_shares(get_user_id(), nicknames);
      } else if (action == "edit") {
        redirect(request, response, get_uri_prefix() + "/sharing/edit?nickname=" +
                 UriUtils::uri_encode(nicknames.front(), false));
        return;
      } else if (action == "activate") {
        dao.activate_track_shares(get_user_id(), nicknames);
      } else if (action == "de-activate") {
        dao.deactivate_track_shares(get_user_id(), nicknames);
      } else {
        throw BadRequestException("Invalid action \"" + action + "\"");
      }
    }
  }
  const std::string user_id = get_user_id();
  auto total_count = dao.get_track_sharing_count_by_user_id(user_id);
  const std::map<std::string, std::string> dummy_map;
  Pagination pagination(get_uri_prefix() + "/sharing", dummy_map, total_count);
  const std::string page = request.get_query_param("page");
  if (!page.empty())
    pagination.set_current_page(std::stoul(page));
  auto track_shares = dao.get_track_sharing_by_user_id(user_id,
                                                       pagination.get_offset(),
                                                       pagination.get_limit());
  build_form(response, pagination, track_shares);
}
