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
#include "tracking_request_handler.hpp"
#include "tracking_download_handler.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <locale>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>
// #ifdef HAVE_CXX17
// #include <optional>
// #endif

using namespace boost::locale;
using namespace fdsd::utils;
using namespace fdsd::trip;
using namespace fdsd::web;
using json = nlohmann::json;

const std::string TrackingRequestHandler::tracking_url = "/tracks";

TrackingRequestHandler::TrackingRequestHandler(std::shared_ptr<TripConfig> config) :
  TripAuthenticatedRequestHandler(config)
{
}

void TrackingRequestHandler::do_preview_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  // Title tor the location tracking page
  set_page_title(translate("Tracks"));
  set_menu_item(tracks);
}

void TrackingRequestHandler::build_form(
    HTTPServerResponse& response,
    bool first_time,
    const Pagination& pagination,
    const TrackPgDao::location_search_query_params& query_params,
    const TrackPgDao::nickname_result& nickname_result,
    const TrackPgDao::tracked_locations_result& locations_result) const
{
  response.content <<
    "<div class=\"container-fluid\">\n"
    // The title of the location tracking page
    "  <h1 class=\"pt-2\">" << translate("Tracks") << "</h1>\n";

  if (!(first_time && locations_result.locations.empty())) {
    response.content <<
      "  <div id=\"location-count\" class=\"card mb-3\">\n"
      "    <div class=\"card-body\">\n"
      // Message showing the location tracking search result
      "      <h3 class=\"card-title\">" << as::number << format(translate("{1} track point received", "{1} track points received", locations_result.total_count)) % locations_result.total_count << "</h3>\n"
      // Message showing the date range for the location tracking search result
      "      <p class=\"card=text\">" << as::datetime << format(translate("Between {1} and {2}")) % locations_result.date_from % locations_result.date_to << "</p>\n"
      "    </div>\n"
      "  </div>\n";

    if (!locations_result.locations.empty()) {
      response.content <<
        "  <div id=\"div-locations-table\" class=\"table-responsive\">\n"
        "    <table id=\"track-points-table\" class=\"table table-striped\">\n"
        // The column heading for the database unique numeric identifier of a tracked location point
        "      <tr><th class=\"text-end\">" << translate("ID") << "</th>"
        // The column heading for the date and time of a tracked location point
        "<th class=\"text-start\">" << translate("Time") << "</th>"
        // The column heading for the latitude coordinate of a tracked location point
        "<th class=\"text-end\">" << translate("Latitude") << "</th>"
        // The column heading for the longitude coordinate of a tracked location point
        "<th class=\"text-end\">" << translate("Longitude") << "</th>"
        // The column heading for the Horizontal Dilution Of Precision (estimated accuracy in metres) of a tracked location point
        "<th class=\"text-end\">HDOP</th>"
        // The column heading for the altitude of a tracked location point
        "<th class=\"text-end\">" << translate("Altitude") << "</th>"
        // The column heading for the speed of a tracked location point
        "<th class=\"text-end\">" << translate("Speed") << "</th>"
        // The column heading for the bearing (angle of heading) of a tracked location point
        "<th class=\"text-end\">" << translate("Bearing") << "</th>"
        // The column heading of a user provided note for the tracked location point
        "<th class=\"text-start\">" << translate("Note") << "</th>"
        // The column heading indicating how the location point was provided, e.g. iPhone, Apple Watch, GPS, manually, mobile phone mast
        "<th class=\"text-start\">" << translate("Provider") << "</th>"
        // The column heading indicating how many satellites were used to determine the location
        "<th class=\"text-end\">" << translate("Satellites") << "</th>"
        // The column heading showing the remaining battery percentage recorded at the time of the tracked location point
        "<th class=\"text-end\">" << translate("Battery") << "</th></tr>\n"
        ;
      for (auto const& location : locations_result.locations) {
        response.content <<
          "      <tr>\n"
          "        <td class=\"text-end\">" << as::number << std::setprecision(0) << location.id.second << "</td>\n";
        const auto date = std::chrono::duration_cast<std::chrono::seconds>(
            location.time_point.time_since_epoch()
          ).count();
        response.content <<
          "        <td>" << as::ftime("%a") << date << " "
                         << as::date_medium << as::datetime << date << "</td>\n";
        response.content << as::posix <<
          "        <td class=\"text-end\"><a href=\"" << get_uri_prefix() << "/map-point?lat=" << std::fixed << std::setprecision(6) << location.latitude << "&lng=" << location.longitude << "\">" << location.latitude << "</a></td>\n"
          "        <td class=\"text-end\"><a href=\"" << get_uri_prefix() << "/map-point?lat=" << location.latitude << "&lng=" << location.longitude << "\">" << location.longitude << "</a></td>\n"
          "        <td class=\"text-end\">";

        if (location.hdop.first)
          response.content << as::number << std::fixed << std::setprecision(1) << location.hdop.second;

        response.content << "</td>\n"
          "        <td class=\"text-end\">";

        if (location.altitude.first)
          response.content << std::fixed << std::setprecision(0) << location.altitude.second;

        response.content << "</td>\n"
          "        <td class=\"text-end\">";

        if (location.speed.first)
          response.content << std::fixed << std::setprecision(1) << location.speed.second;

        response.content << "</td>\n"
          "        <td class=\"text-end\">";

        if (location.bearing.first)
          response.content << std::fixed << std::setprecision(0) << location.bearing.second;

        response.content << "</td>\n"
          "        <td class=\"text-start\">" << (location.note.first ? x(location.note.second) : "") << "</td>\n"
          "        <td class=\"text-start\">" << (location.provider.first ? x(location.provider.second) : "") << "</td>\n"
          "        <td class=\"text-end\">";

        if (location.satellite_count.first)
          response.content << std::fixed << std::setprecision(0) << location.satellite_count.second;

        response.content << "</td>\n"
          "        <td class=\"text-end\">";

        if (location.battery.first)
          response.content << std::fixed << std::setprecision(1) << location.battery.second;

        response.content << "</td>\n"
          "      </tr>\n";
      } // for locations_result.locations

      response.content
        <<
        "    </table>\n"
        "  </div>\n";

      if (pagination.get_page_count() > 1) {
        response.content
          <<
          "  <div id=\"div-paging\" class=\"pb-0\">\n"
          << pagination.get_html()
          <<
          "  </div>\n";
      }
    } // if (!locations_result.locations.empty())
  } // if first_time

  response.content <<
    "  <form name=\"form\">\n";

  // Inside the form, but not the card div, so it benefits from the form
  // submission but renders more associatively with the pagination div.
  const auto page_count = pagination.get_page_count();
  if (!first_time && page_count > 1) {
    response.content
      <<
      "    <div class=\"d-flex justify-content-center pt-0 pb-0 col-12\">\n"
      "      <input id=\"goto-page\" type=\"number\" name=\"page\" value=\""
      << std::fixed << std::setprecision(0) << pagination.get_current_page()
      << "\" min=\"1\" max=\"" << page_count << "\">\n"
      // Title of button which goes to a specified page number
      "      <button id=\"goto-page-btn\" class=\"btn btn-sm btn-primary\" type=\"submit\" name=\"action\" accesskey=\"g\" value=\"goto-page\">" << translate("Go") << "</button>\n"
      "    </div>\n"
      ;
  }

  response.content <<
    "    <div class=\"container-fluid bg-light row g-3 my-3 pb-3 mx-0\">\n";

  response.content << as::posix <<
    // "      <div class=\"row\">\n"
    "      <div class=\"col-lg-3\">\n"
    // The input start date to search for location tracks
    "        <label for=\"input-date-from\" class=\"form-label\">" << translate("Date from") << "</label>\n"
    "        <input id=\"input-date-from\" class=\"form-control\"  aria-describedby=\"validationFromDate\" type=\"datetime-local\" name=\"from\" value=\"" << x(query_params.datetime_as_html_input_value(query_params.date_from)) <<  "\" size=\"25\" step=\"1\" required >\n"
    "        <div id=\"validationFromDate\" class=\"invalid-feedback\">Enter the start date to fetch points from</div>\n"
    "      </div>\n"
    "      <div class=\"col-lg-3\">\n"
    // The input end date to search for location tracks
    "        <label for=\"input-date-to\" class=\"form-label\">" << translate("Date to") << "</label>\n"
    "        <input id=\"input-date-to\" class=\"form-control\" aria-describedby=\"validationFromDate\" type=\"datetime-local\" name=\"to\" value=\"" << x(query_params.datetime_as_html_input_value(query_params.date_to)) << "\" size=\"25\" step=\"1\" required >\n"
    "        <div id=\"validationToDate\" class=\"invalid-feedback\">Enter the end date to fetch points until</div>\n"
    // "      </div>\n"
    "      </div>\n";

  // Don't show the select element if there are no nicknames sharing with this user
  if (!nickname_result.nicknames.empty()) {
    response.content <<
      "      <div id=\"div-nicknames\" class=\"col-sm-3\">\n"
      // The input to select another user's nickname to view their shared location tracks
      "        <label for=\"nicknameSelect\" class=\"form-label\">" << translate("Display shared user's tracks") << "</label>\n"
      "        <select id=\"nicknameSelect\" class=\"form-select\" name=\"nickname\" aria-label=\"Select nickname\">\n";
    // The search for the current user's nickname should never fail, but...
    if (!nickname_result.nickname.empty()) {
      // The current logged in user
      response.content <<
        "          <option value=\"\"";
      if (query_params.nickname.empty())
        response.content << " selected";
      response.content <<
        ">" << x(nickname_result.nickname) << "</option>\n";
    }

    for (auto const& nickname : nickname_result.nicknames) {
      response.content <<
        "          <option value=\"" << x(nickname) << "\"";
      if (query_params.nickname == nickname)
        response.content << " selected";
      response.content <<
        ">" << x(nickname) << "</option>\n";
    }
    response.content <<
      "        </select>\n"
      "      </div><!-- div-nicknames -->\n";
  }

  response.content <<
    "      <div class=\"col-sm-3 pb-2\">\n"
    // The maximum Horizontal Dilution of Precision (accuracy in metres) to filter locations by
    "        <label for=\"input-max-hdop\" class=\"form-label\">" << translate("Max hdop") << "</label>\n"
    "        <input id=\"input-max-hdop\" class=\"form-control\" type=\"number\" min=\"0\" max=\"9999\" name=\"max_hdop\" value=\"";
  if (query_params.max_hdop >= 0) {
    response.content << query_params.max_hdop;
  }
  response.content <<
    // instructions to disable filtering locations by HDOP (accuracy in metres)
    "\"> (" << translate("leave blank for no maximum") << ")\n"
    "      </div>\n"
    "      <div class=\"col-12\">\n"
    "        <div class=\"form-check\">\n"
    "          <input id=\"input-notes-only\" class=\"form-check-input\" type=\"checkbox\" name=\"notes_only_flag\"";
  if (query_params.notes_only_flag)
    response.content << " checked";
  response.content <<
    ">\n"
    // Whether the user wishes to filter the results to only include those with notes attached
    "          <label for=\"input-notes-only\" class=\"form-check-label\">" << translate("Show notes only") << "</label>\n"
    "        </div>\n"
    "      </div>\n"
    "      <div class=\"col-12 pt-3\" aria-label=\"Form buttons\">"
    // Label for the button to perform the search that lists the tracked locations
    "        <button id=\"btn-tracks\" type=\"submit\" name=\"action\" value=\"list\" accesskey=\"l\" class=\"btn btn-lg btn-success\">" << translate("List tracks") << "</button>\n"
    // Label for the button to display a map showing the tracked locations
    "        <button id=\"btn-map\" type=\"submit\" formaction=\"" << get_uri_prefix() << "/map\" name=\"action\" value=\"show_map\" accesskey=\"m\" class=\"btn btn-lg btn-primary\">" << translate("Show map") << "</button>\n"
    "        <button id=\"btn-download\" type=\"submit\" formaction=\"" << get_uri_prefix() << TrackingDownloadHandler::tracking_download_url << "\" name=\"action\" value=\"download\" accesskey=\"d\" class=\"btn btn-lg btn-success\"\n"
    // Prompt to confirm the user wished to download a data file with the tracked locations
    "         onclick=\"return confirm('" << translate("Download tracks?") << "');\">"
    // Label for the button to download the tracks as an XML data file
                   << translate("Download tracks") << "</button>\n"
    "        <!--\n"
    // Label for the button to make a copy of the tracked locations
    "        <button id=\"btn-copy\" type=\"submit\" name=\"action\" value=\"copy\" accesskey=\"y\" class=\"btn btn-lg btn-primary\">" << translate("Copy") << "</button>\n"
    "        -->\n"
    // Label for the button which resets the form's input criteria to that originally displayed
    "        <button id=\"btn-reset\" type=\"submit\" name=\"action\" value=\"reset\" accesskey=\"r\" class=\"btn btn-lg btn-danger\">" << translate("Reset") << "</button>\n"
    "      </div>\n"
    "    </div><!-- container -->\n"
    "  </form>\n"
    "</div>\n"
    ;
}

void TrackingRequestHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  auto query_params = request.get_query_params();
  bool first_time = query_params.empty();
  TrackPgDao::location_search_query_params q;
  q.user_id = get_user_id();

  SessionPgDao session_dao;
  if (first_time) {
    try {
      std::string json_str =
        session_dao.get_value(get_session_id(), SessionPgDao::tracks_query_key);
      if (!json_str.empty()) {
        json j = json::parse(json_str);
        q = j.get<TrackPgDao::location_search_query_params>();
        first_time = false;
        // std::cout << "Read query from session: " << j.dump(4) << '\n';
      }
    } catch (const std::exception& e) {
      std::cerr << "Failed to fetch query parameters from session\n"
                << e.what() << '\n';
    }
  } else {
    // std::cout << "Query params\n";
    // for (auto const& p : request.query_params) {
    //   std::cout << p.first << " -> " << p.second << '\n';
    // }

    q = TrackPgDao::location_search_query_params(get_user_id(),
                                                 query_params);

    // std::cout << "Fetched query from URL as: " << q << '\n';
    // This is user supplied data, so serialization could fail
    try {
      json j = q;
      // std::cout << "Saving JSON: " << j.dump(4) << '\n';
      session_dao.save_value(get_session_id(),
                             SessionPgDao::tracks_query_key,
                             j.dump());
    } catch (const std::exception& e) {
      std::cerr << "Failed to save query parameters in session\n"
                << e.what() << '\n';
    }
  }

  const std::string action = q.get_value(query_params, "action");
  if (action == "list") {
    // New query, so reset page number
    q.page = 1;
  } else if (action == "reset") {
    q = TrackPgDao::location_search_query_params{};
    q.user_id = get_user_id();
  }

  // std::cout << "Query object: " << q << "\n- - -\n";
  // for (auto const& p : q.query_params()) {
  //   std::cout << p.first << " -> " << p.second << '\n';
  // }

  TrackPgDao dao;
  TrackPgDao::tracked_locations_result locations_result;

  Pagination pagination(get_uri_prefix() + tracking_url,
                        q.query_params());
  if (!first_time) {
    // std::cout << "Query before pagination: " << q << '\n';
    pagination.set_current_page(q.page);
    q.order = dao_helper::descending;
    q.page_offset = pagination.get_offset();
    q.page_size = pagination.get_limit();

    locations_result = dao.get_tracked_locations(q);
    pagination.set_total(locations_result.total_count);

    // This occurs where the query parameters are changed and the user selects
    // the show map option.  If the number of pages for the new query are less
    // than the saved page number, it's messy.  So, repeat the query, fetching
    // the first page.
    if (q.page > 1 && locations_result.locations.empty()) {
      q.page = 1;
      pagination.set_current_page(q.page);
      q.page_offset = pagination.get_offset();
      q.page_size = pagination.get_limit();
      locations_result = dao.get_tracked_locations(q);
      pagination.set_total(locations_result.total_count);
    }

    // std::cout << "Fetched "
    //           << locations_result.locations.size()
    //           << " locations from "
    //           << locations_result.total_count
    //           << " total\n";

  }
  auto nickname_result = dao.get_nicknames(get_user_id());
  build_form(response, first_time, pagination, q, nickname_result, locations_result);
}
