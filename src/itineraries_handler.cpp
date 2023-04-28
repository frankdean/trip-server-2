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
#include "itineraries_handler.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/pagination.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>
#include <map>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using json = nlohmann::json;

void ItinerariesHandler::build_page(
    HTTPServerResponse& response,
    const Pagination& pagination,
    const std::vector<ItineraryPgDao::itinerary_summary> itineraries)
{
  response.content
    <<
    "<div class=\"container-fluid\">\n"
    // Page title of the itenerary list page
    "  <h1 class=\"pt-2\">" << translate("Itineraries") << "</h1>\n";
  if (itineraries.empty()) {
    response.content
      <<
      "  <div class=\"alert alert-info\"\n>"
      // Message displayed when there are no itineries belonging to or shared to a user
      "    <p>" << translate("There are no itineraries to display") << "</p>\n"
      "  </div>\n";
  } else {
    response.content
      <<
      "  <div id=\"itineraries\" class=\"table-responsive\">\n"
      "    <table id=\"table-itineraries\" class=\"table table-striped\">\n"
      "      <tr>\n"
      // Column heading showing the start dates of each itinerary in the list
      "        <th>" << translate("Start Date") << "</th>\n"
      // Column heading showing the title of each each itinerary in the list
      "        <th>" << translate("Title") << "</th>\n"
      // Column heading showing the owner's nickname of each itinerary in the list
      "        <th>" << translate("Owner") << "</th>\n"
      // Colunn heading indicating whether the itinerary is shared with one or more other users
      "        <th>" << translate("Sharing") << "</th>\n"
      "      </tr>\n";
    for (const auto it : itineraries) {
      response.content
        <<
        "      <tr>\n"
        "        <td>";
      const auto date = std::chrono::duration_cast<std::chrono::seconds>(
          it.start.second.time_since_epoch()
        ).count();
      if (it.start.first) {
        response.content
          << as::ftime("%a") << date << " "
          << as::date_medium << as::date << date
          << as::posix;
      }
      response.content
        << "</td>\n"
        "        <td><a href=\"" << get_uri_prefix() << "/itinerary?id=" << it.id.second << "\">" << x(it.title) << "</a></td>\n"
        "        <td>" << (it.owner_nickname.first ? x(it.owner_nickname.second) : "") << "</td>\n"
        "        <td>" << (it.shared.first && it.shared.second ? "&check;" : "") << "</td>\n";
    } // for
    response.content
      <<
      "      </tr>\n"
      "    </table>\n"
      "  </div>\n"; // #itineraries div
  }
  response.content
    <<
    "  <div id=\"div-buttons\">\n"
    "    <form name=\"form\" class=\"css-form\">\n";

  const auto page_count = pagination.get_page_count();
  if (page_count > 1) {
    response.content
      <<
      "    <div id=\"div-paging\" class=\"pb-0\">\n"
      << pagination.get_html()
      <<
      "    </div>\n"
      "    <div class=\"d-flex justify-content-center pt-0 pb-0 col-12\">\n"
      "      <input id=\"goto-page\" type=\"number\" name=\"page\" value=\""
      << std::fixed << std::setprecision(0) << pagination.get_current_page()
      << "\" min=\"1\" max=\"" << page_count << "\">\n"
      // Title of button which goes to a specified page number
      "      <button id=\"goto-page-btn\" class=\"btn btn-sm btn-primary\" type=\"submit\" name=\"action\" accesskey=\"g\" value=\"goto-page\">" << translate("Go") << "</button>\n"
      "    </div>\n"
      ;
  }

  response.content
    <<
    "      <div id=\"itineraries-div-form-buttons\" class=\"py-2\">\n"
    // Button title for creating something new
    "        <button id=\"btn-new\" formaction=\"" << get_uri_prefix() << "/itinerary/edit\" type=\"submit\" accesskey=\"w\" class=\"btn btn-lg btn-warning\">" << translate("New") << "</button>\n"
    // Button title for importing something
    "        <button id=\"btn-import\" accesskey=\"i\" name=\"action\" value=\"import\" formmethod=\"get\" formaction=\"" << get_uri_prefix() << "/itinerary/import\" class=\"btn btn-lg btn-success\">" << translate("Import") << "</button>\n"
    "      </div>\n";
  if (!itineraries.empty()) {
    response.content
      <<
      "    <div class=\"py-2\">\n"
      // Button title for searching for something
      "      <button id=\"btn-search\" accesskey=\"s\" class=\"btn btn-lg btn-primary\" formmethod=\"get\" formaction=\"" << get_uri_prefix() << "/itinerary-search\">" << translate("Search") << "</button>\n"
      // Button title for navigating to the Itinerary Sharing Report
      "      <button id=\"btn-shares-report\" accesskey=\"r\" class=\"btn btn-lg btn-primary\" formmethod=\"get\" formaction=\"" << get_uri_prefix() << "/itinerary-sharing-report\">" << translate("Itinerary shares report") << "</button>\n"
      "    </div>\n";
  }
  response.content
    <<
    "    </form>\n"
    "  </div>\n"
    "</div>\n";
}

void ItinerariesHandler::do_preview_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  set_page_title(translate("Itineraries"));
  set_menu_item(itineraries);
}

void ItinerariesHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  try {
    ItineraryPgDao dao;
    const std::string user_id = get_user_id();
    const long total_count = dao.get_itineraries_count(user_id);
    const std::map<std::string, std::string> dummy_map;
    Pagination pagination(get_uri_prefix() + "/itineraries",
                          dummy_map,
                          total_count);
    std::string page = request.get_param("page");
    SessionPgDao session_dao;
    if (page.empty()) {
      // Use JSON object so we can easily extend in the future,
      // e.g. filtering, sorting etc.
      try {
        const auto json_str = session_dao.get_value(
            get_session_id(),
            SessionPgDao::itinerary_page_key);
        if (!json_str.empty()) {
          json j = json::parse(json_str);
          page = j["page"];
        }
      } catch (const nlohmann::detail::parse_error &e) {
        std::cerr << "JSON parse error parsing session value for "
                  << SessionPgDao::itinerary_page_key
                  << " key\n";
      }
    } else {
      json j;
      // if (j.contains("page"))
      j["page"] = page;
      session_dao.save_value(get_session_id(),
                             SessionPgDao::itinerary_page_key,
                             j.dump());
    }
    try {
      if (!page.empty())
        pagination.set_current_page(std::stoul(page));
    } catch (const std::logic_error& e) {
      std::cerr << "Error converting string to page number\n";
    }
    auto itineraries = dao.get_itineraries(user_id,
                                           pagination.get_offset(),
                                           pagination.get_limit());
    build_page(response, pagination, itineraries);
  } catch (const std::exception &e) {
    std::cerr << "Exception handling request for a list of itineraries: "
              << e.what() << '\n';
    throw;
  }
}
