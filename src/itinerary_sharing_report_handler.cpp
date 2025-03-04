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
#include "itinerary_sharing_report_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;

void ItinerarySharingReportHandler::build_form(
    std::ostream &os,
    const web::Pagination& pagination,
    const std::vector<ItineraryPgDao::itinerary_share_report> itineraries)
{
  os <<
    "<div class=\"container-fluid my-3\">\n"
    "  <h1>" << get_page_title() << "</h1>\n";
  if (itineraries.empty()) {
    os <<
      "  <div class=\"alert alert-info\" ng-show=\"itineraries.length === 0\">\n"
      // Message shown when there are no itineraries for the itinerary sharing report
      "    <p>" << translate("You are not currently sharing any itineraries") << "</p>\n"
      "  </div>\n";
  } else {
    os <<
      "  <form>\n"
      "    <table id=\"table-itineraries\" class=\"table table-striped\">\n"
      "      <tr>\n"
      // Column heading showing the start dates of each itinerary in the list
      "        <th>" << translate("Start Date") << "</th>\n"
      // Column heading showing the title of each each itinerary in the list
      "        <th>" << translate("Title") << "</th>\n"
      // Colunn heading showing which nicknames and itinerary is shared with
      "        <th>" << translate("Sharing with") << "</th>\n"
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
        "        <td><a href=\"" << get_uri_prefix() << "/itinerary-sharing?id=" << it.id.value() << "&routing=itinerary-sharing-report&report-page=" << pagination.get_current_page() << "\">" << x(it.title) << "</a></td>\n"
        "        <td>";
      for (const auto &nickname : it.nicknames) {
        os << nickname << ' ';
      }
      os << "</td>\n";
    } // for
    os
      <<
      "      </tr>\n"
      "    </table>\n";
    const auto page_count = pagination.get_page_count();
    if (page_count > 1) {
      os
        <<
        "      <div id=\"div-paging\" class=\"pb-0\">\n"
        << pagination.get_html()
        <<
        "      </div>\n"
        "      <div class=\"d-flex justify-content-center pt-0 pb-0 col-12\">\n"
        "        <input id=\"page\" type=\"number\" name=\"report-page\" value=\""
        << std::fixed << std::setprecision(0) << pagination.get_current_page()
        << "\" min=\"1\" max=\"" << page_count << "\">\n"
        // Title of button which goes to a specified page number
        "        <button id=\"goto-page-btn\" class=\"btn btn-sm btn-primary\" name=\"action\" accesskey=\"g\" value=\"page\">" << translate("Go") << "</button>\n"
        "      </div>\n"
        ;
    }
    os <<
      "  </form>\n";
  }
  os <<
    "</div>\n";
}

void ItinerarySharingReportHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  (void)request; // unused
  (void)response;
  // Page title of the itinerary shares report page
  set_page_title(translate("Itinerary Shares Report"));
  set_menu_item(unknown);
}

void ItinerarySharingReportHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  ItineraryPgDao dao;
  const std::string user_id = get_user_id();

  const long total_count = dao.get_shared_itinerary_report_count(user_id);
  std::map<std::string, std::string> page_param_map;
  Pagination pagination(get_uri_prefix() + "/itinerary-sharing-report",
                        page_param_map,
                        total_count,
                        10,
                        5,
                        true,
                        true,
                        "report-page");
  const auto page = request.get_param("report-page");
  if (!page.empty())
    pagination.set_current_page(std::stoul(page));
  auto itineraries = dao.get_shared_itinerary_report(
      user_id,
      pagination.get_offset(),
      pagination.get_limit());
  build_form(response.content, pagination, itineraries);
}
