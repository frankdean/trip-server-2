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
#include "itinerary_search_results_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;
using json = nlohmann::json;

const int ItinerarySearchResultsHandler::max_search_radius_kilometers = 50;

void ItinerarySearchResultsHandler::build_form(
    std::ostream &os,
    const Pagination& pagination,
    const std::vector<ItineraryPgDao::itinerary_summary> itineraries)
{
  os
    <<
    "<div class=\"container-fluid\">\n";
  os << "  <h1>" << get_page_title() << "</h1>\n";
  if (error_message.has_value()) {
    os
      << "  <div class=\"alert alert-danger\">\n"
      << "    <p>" << x(error_message)
      << "</p>\n  </div>\n";
  }
  if (itineraries.empty()) {
    os
      <<
      "  <div class=\"alert alert-info\"\n>"
      // Message displayed when no itineries match the itinerary search by radius criteria
      "    <p>" << translate("No itineraries found.") << "</p>\n"
      "  </div>\n";
  } else {
    os
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
    for (const auto &it : itineraries) {
      os
        <<
        "      <tr>\n"
        "        <td>";
      if (it.start.has_value()) {
        const auto date = std::chrono::duration_cast<std::chrono::seconds>(
            it.start.value().time_since_epoch()
          ).count();
        os
          << as::ftime("%a") << date << " "
          << as::date_medium << as::date << date
          << as::posix;
      }
      os
        << "</td>\n"
        "        <td><a href=\"" << get_uri_prefix() << "/itinerary?id=" << it.id.value() << "\">" << x(it.title) << "</a></td>\n"
        "        <td>" << (it.owner_nickname.has_value() ? x(it.owner_nickname.value()) : "") << "</td>\n"
        "        <td>" << (it.shared.has_value() && it.shared.value() ? "&check;" : "") << "</td>\n";
    } // for
    os
      <<
      "      </tr>\n"
      "    </table>\n"
      "  </div>\n" // #itineraries div
      "  <div id=\"div-buttons\">\n"
      "    <form name=\"form\">\n"
      "    <input type=\"hidden\" name=\"lng\" value=\"" << longitude  << "\">\n"
      "    <input type=\"hidden\" name=\"lat\" value=\"" << latitude  << "\">\n"
      "    <input type=\"hidden\" name=\"radius\" value=\"" << radius  << "\">\n";
    const auto page_count = pagination.get_page_count();
    if (page_count > 1) {
      os
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
    os
      <<
      "    </form>\n"
      "  </div>\n"
      "</div>\n";
  }
}

void ItinerarySearchResultsHandler::do_preview_request(
    const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response)
{
  (void)request; // unused
  (void)response;
  // Page title of the itinerary search results page
  set_page_title(translate("Itinerary Search Results"));
  set_menu_item(unknown);
}

void ItinerarySearchResultsHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  if (request.method == HTTPMethod::post) {
    // Save the selected position formatting to the user's session
    const auto coord_format = request.get_post_param("coordFormat");
    const auto position_format = request.get_post_param("positionFormat");
    if (!coord_format.empty() && !position_format.empty()) {
      SessionPgDao session_dao;
      json j;
      j["coord_format"] = coord_format;
      j["position_format"] = position_format;
      session_dao.save_value(get_session_id(),
                             SessionPgDao::coordinate_format_key,
                             j.dump());
    }
  }
  try {
    longitude = std::stod(request.get_param("lng"));
    latitude = std::stod(request.get_param("lat"));
  } catch (const std::invalid_argument &e) {
    error_message = translate("Invalid position");
  }
  radius = std::stod(request.get_param("radius"));
  if (radius > max_search_radius_kilometers)
    throw BadRequestException("Radius for itinerary search exceeds maximum");
  const double radius_meters = radius * 1000;
  // std::cout << "Searching for lat: "
  //           << std::fixed << std::setprecision(4)
  //           << latitude << ", lng: " << longitude
  //           << " with radius of " << radius_meters << " meters\n";
  ItineraryPgDao dao;
  const std::string user_id = get_user_id();
  const long total_count = dao.itinerary_radius_search_count(
      user_id,
      longitude,
      latitude,
      radius_meters);
  // std::cout << "Search result count: " << total_count << '\n';
  std::map<std::string, std::string> query_map;
  query_map["lng"] = std::to_string(longitude);
  query_map["lat"] = std::to_string(latitude);
  query_map["radius"] = std::to_string(radius);
  Pagination pagination(get_uri_prefix() + "/itinerary-search-result",
                        query_map,
                        total_count);
  std::string page = request.get_param("page");
  SessionPgDao session_dao;
  if (page.empty()) {
    // Use JSON object so we can easily extend in the future,
    // e.g. filtering, sorting etc.
    try {
      const auto json_str = session_dao.get_value(
          get_session_id(),
          SessionPgDao::itinerary_radius_search_page_key);
      if (!json_str.empty()) {
        json j = json::parse(json_str);
        page = j["page"];
      }
    } catch (const nlohmann::detail::parse_error &e) {
      std::cerr << "JSON parse error parsing session value for "
                << SessionPgDao::itinerary_radius_search_page_key
                << " key\n";
    }
  } else {
    json j;
    // if (j.contains("page"))
    j["page"] = page;
    session_dao.save_value(get_session_id(),
                           SessionPgDao::itinerary_radius_search_page_key,
                           j.dump());
  }
  try {
    if (!page.empty())
      pagination.set_current_page(std::stoul(page));
  } catch (const std::logic_error& e) {
    std::cerr << "Error converting string to page number\n";
  }
  // auto itineraries = dao.get_itineraries(
  //     user_id,
  //     pagination.get_offset(),
  //     pagination.get_limit());
  auto itineraries = dao.itinerary_radius_search(
      user_id,
      longitude,
      latitude,
      radius_meters,
      pagination.get_offset(),
      pagination.get_limit());
  build_form(response.content, pagination, itineraries);
}
