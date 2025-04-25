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
#include "track_sharing_edit_handler.hpp"
#include "track_sharing_handler.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <sstream>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

void TrackSharingEditHandler::do_preview_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  (void)response; // unused
  is_new = (request.get_param("new") == "true");
  nickname = request.get_param("nickname");
  dao_helper::trim(nickname);
  if (is_new) {
    // Title of the page when creating the sharing of tracked locations with another user
    set_page_title(translate("Track Sharing&mdash;New"));
  } else {
    // Title of the page when editing the sharing of tracked locations with another user
    set_page_title(translate("Track Sharing&mdash;Edit"));
  }

  set_menu_item(unknown);
}

void TrackSharingEditHandler::build_form(
    HTTPServerResponse& response,
    const TrackPgDao::location_share_details& share) const
{
  std::optional<period_dhm> recent_dhm(share.recent_minutes);
  std::optional<period_dhm> max_dhm(share.max_minutes);

  response.content
    <<
    "  <div class=\"container-fluid\">\n";
  append_messages_as_html(response.content);
  response.content
    <<
    "    <form name=\"form\" method=\"post\">\n"
    "      <input type=\"hidden\" name=\"page\" value=\"" << current_page << "\">\n"
    "      <div id=\"div-nickname\" class=\"container-fluid bg-light py-3 my-3\">\n"
    // Prompt to input the nickname to share location tracking with
    "        <label for=\"input-nickname\">" << translate("Share location with nickname") << "</label>\n"
    "        <input id=\"input-nickname\" name=\"nickname\"";
  if (!is_new) {
    response.content << " disabled";
  }
  response.content
    <<
    " value=\"" << x(nickname) << "\" required autofocus>\n"
    "      </div>\n"
    "      <div id=\"div-recent-time\" class=\"container-fluid bg-light py-3 my-3\">\n"
    // Prompt when entering recent tracking criteria when sharing locations
    "        " << translate("Share locations within last") << "\n"
    "        <input id=\"input-days\" name=\"recentDays\" type=\"number\" min=\"0\"\n"
    "               max=\"99999\" class=\"days\" ";
  if (recent_dhm.has_value())
    response.content << " value=\"" << recent_dhm.value().days << '"';
  response.content
    <<
    ">\n"
    // Label for input of days
    "        <label for=\"input-days\">" << translate("days") << "</label>\n"
    "        <input id=\"input-hours\" name=\"recentHours\" type=\"number\" min=\"0\"\n"
    "               max=\"23\" class=\"hours\"";
  if (recent_dhm.has_value())
    response.content << " value=\"" << recent_dhm.value().hours << '"';
  response.content
    <<
    ">\n"
    // Label for input of hours
    "        <label for=\"input-hours\">" << translate("hours") << "</label> and\n"
    "        <input id=\"input-minutes\" name=\"recentMinutes\" type=\"number\" min=\"0\"\n"
    "               max=\"59\" class=\"minutes\"";
  if (recent_dhm.has_value())
    response.content << " value=\"" << recent_dhm.value().minutes << '"';
  response.content
    <<
    ">\n"
    // Label for input of minutes
    "        <label for=\"input-minutes\">" << translate("minutes") << "</label>\n"
    // label describing the meaning of a recently logged location
    "        " << translate("of most recently logged location (Set to zero or leave blank for no restriction)") << "\n"
    "      </div>\n"
    "      <div id=\"div-max-time\" class=\"container-fluid bg-light py-3 my-3\">\n"
    // Prompt when entering maximum recent tracking criteria when sharing locations
    "        " << translate("Limit sharing to a maximum period of") << "\n"
    "        <input id=\"input-max-days\" name=\"maxDays\" type=\"number\" min=\"0\"\n"
    "               max=\"99999\" class=\"days\"";
  if (max_dhm.has_value())
    response.content << " value=\"" << max_dhm.value().days << '"';
  response.content
    <<
    ">\n"
    // Label for input of days
    "        <label for=\"input-max-days\">" << translate("days") << "</label>\n"
    "        <input id=\"input-max-hours\" name=\"maxHours\" type=\"number\" min=\"0\"\n"
    "               max=\"23\" class=\"hours\"";
  if (max_dhm.has_value())
    response.content << " value=\"" << max_dhm.value().hours << '"';
  response.content
    <<
    ">\n"
    // Label for input of hours
    "        <label for=\"input-max-hours\">" << translate("hours") << "</label> and\n"
    "        <input id=\"input-max-minutes\" name=\"maxMinutes\" type=\"number\" min=\"0\"\n"
    "               max=\"59\" class=\"minutes\"";
  if (max_dhm.has_value())
    response.content << " value=\"" << max_dhm.value().minutes << '"';
  response.content
    <<
    ">\n"
    // Label for input of minutes
    "        <label for=\"input-max-minutes\">" << translate("minutes") << "</label>\n"
    // label describing the meaning of a maximum recently logged location
    "        " << translate("in any event. (Set to zero, or leave blank for no restriction)") << "\n"
    "      </div>\n"
    "      <div id=\"div-active\" class=\"container-fluid bg-light py-3 my-3\">\n"
    // Label for checkbox indicating whether location sharing for a particular nickname is active
    "        <label for=\"input-active\" class=\"pe-1\">" << translate("Active") << "</label><input id=\"input-active\" type=\"checkbox\" name=\"active\" accesskey=\"a\"";
  if (share.active.has_value() && share.active.value())
    response.content << " checked";
  response.content
    <<
      ">\n"
    "      </div>\n"
    "      <div id=\"div-form-buttons\" class=\"py-3 my-3\">\n"
    "        <input type=\"hidden\" name=\"new\" value=\"" << (is_new ? "true" : "false") << "\">\n"
    // Label for a save button
        "        <button id=\"btn-save\" type=\"submit\" name=\"action\" value=\"save\" accesskey=\"s\" class=\"btn btn-lg btn-primary\">" << translate("Save") << "</button>\n"
    // Label for a cancel button
        "        <button id=\"btn-cancel\" type=\"submit\" name=\"action\" value=\"cancel\" accesskey=\"c\" class=\"btn btn-lg btn-danger\" formnovalidate>" << translate("Cancel") << "</button>\n"
    // Label for a reset button
    "        <button id=\"btn-reset\" type=\"reset\" accesskey=\"r\" class=\"btn btn-lg btn-danger\">Reset</button>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n";
}

int TrackSharingEditHandler::convert_dhm_to_minutes(
    std::string days,
    std::string hours,
    std::string minutes) const
{
  try {
    int d = days.empty() ? 0 : std::stoi(days);
    int h = hours.empty() ? 0 : std::stoi(hours);
    int m = minutes.empty() ? 0 : std::stoi(minutes);
    return (d * 24 + h) * 60 + m;
  } catch (const std::exception &e) {
    std::cerr << "Exception converting \"" << days << "\" days, \""
              << hours << "\" hours, \"" << minutes << "\" minutes: "
              << e.what() << '\n';
    throw;
  }
}

void TrackSharingEditHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  const std::string page = request.get_param("page");
  try {
    if (!page.empty())
      current_page = std::stoul(page);
  } catch (const std::logic_error& e) {
    std::cerr << "Error converting string to page number\n";
  }

  const std::string action = request.get_post_param("action");
  TrackPgDao::location_share_details share;

  try {
    if (action == "save") {
      TrackPgDao dao(elevation_service);
      share.shared_by_id = get_user_id();
      try {
        share.shared_to_id = dao.get_user_id_by_nickname(nickname);
        share.active = std::optional<bool>();
        int recent_minutes = convert_dhm_to_minutes(
            request.get_post_param("recentDays"),
            request.get_post_param("recentHours"),
            request.get_post_param("recentMinutes"));
        if (recent_minutes > 0)
          share.recent_minutes = recent_minutes;
        int max_minutes = convert_dhm_to_minutes(
            request.get_post_param("maxDays"),
            request.get_post_param("maxHours"),
            request.get_post_param("maxMinutes"));
        if (max_minutes > 0)
          share.max_minutes = max_minutes;
        const std::string s = request.get_post_param("active");
        if (!s.empty() && s == "on")
          share.active = true;
        dao.save(share);
        std::ostringstream os;
        os << get_uri_prefix() + "/sharing";
        const std::string page = request.get_param("page");
        if (!page.empty())
          os << "?page=" << page;
        redirect(request, response, os.str());
        return;
      } catch (const pqxx::unexpected_rows &e) {
        // Error shown to the user when a nickname does not exist
        add_message(
            UserMessage(translate("Nickname does not exist"),
                        UserMessage::Type::error));
      }
    } else if (action == "cancel") {
      std::ostringstream os;
      os << get_uri_prefix() + "/sharing";
      const std::string page = request.get_param("page");
      if (!page.empty())
        os << "?page=" << page;
      redirect(request, response, os.str());
      return;
    } else {
      if (!nickname.empty()) {
        TrackPgDao dao(elevation_service);
        auto result = dao.get_tracked_location_share_details_by_sharer(nickname, get_user_id());
        if (result.has_value())
          share = result.value();
        else
          share.shared_by_id = get_user_id();
      }
    }
    try {
      build_form(response, share);
    } catch (const std::exception &e) {
      std::cerr << "Exception creating form: " << e.what() << '\n';
      throw;
    }
  } catch (const std::exception &e) {
    std::cerr << "Exception handling request: " << e.what() << '\n';
    throw;
  }
}
