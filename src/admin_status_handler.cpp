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
#include <optional>
#include "admin_status_handler.hpp"
#include "../trip-server-common/src/http_response.hpp"

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;

void AdminStatusHandler::build_form(
    std::ostream &os,
    const SessionPgDao::tile_report &tile_report)
{
  os <<
    "<div class=\"container-fluid my-3\">\n"
    "<h1>" << get_page_title() << "</h1>\n";
  if (tile_report.metrics.empty()) {
    os <<
      "    <div class=\"alert alert-info\">\n"
      // Message displayed when no tile downloads have been recorded
      "      <p>" << translate("No tile usage data has been collected yet") << "</p>\n"
      "    </div>\n";
  } else {
    os <<
      "  <div id=\"tile-usage\">\n"
      "    <div>\n";
    if (tile_report.total.has_value() && tile_report.time.has_value()) {
      const auto date = std::chrono::duration_cast<std::chrono::seconds>(
          tile_report.time.value().time_since_epoch()
        ).count();

      os <<
        // Formatted count of the total number of map tiles downloaded
        "      <p>" << format(translate("Fetched a total of {1,number} tiles, as at {2,ftime='%R %a'} {2,date=medium}")) % tile_report.total.value() % date << "</p>\n";
    }
    os <<
      "      <div>\n"
      "        <table id=\"tile-metrics\" class=\"tile-metric-table table table-striped\">\n"
      "          <tr>\n"
      // Column heading for admin status report for years
      "            <th class=\"text-end\">" << translate("Year") << "</th>\n"
      // Column heading for admin status report for months
      "            <th class=\"text-end\">" << translate("Month") << "</th>\n"
      // Column heading for admin status report for the monthly tile download
      "            <th class=\"text-end\">" << translate("Monthly Usage") << "</th>\n"
      // Column heading for admin status report for the cumulate total tile downloads
      "            <th class=\"text-end\">" << translate("Cumulative Total") << "</th>\n"
      "          </tr>\n";
    for (auto const &t : tile_report.metrics) {
      os <<
        "          <tr>\n"
        "            <td class=\"text-end\">" << t.year << "</td>\n"
        "            <td class=\"text-end\">" << t.month << "</td>\n"
        "            <td class=\"text-end\">" << as::number << t.quantity << "</td>\n"
        "            <td class=\"text-end\">" << t.cumulative_total << as::posix << "</td>\n"
        "          </tr>\n";
    }
    os <<
      "        </table>\n"
      "      </div>\n"
      "    </div>\n"
      "  </div>\n";
  }
  os <<
    "</div>\n";
}

void AdminStatusHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  // Page title of the admin user's User Management page
  set_page_title(translate("System Status"));
  set_menu_item(status);
}

void AdminStatusHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  SessionPgDao session_dao;
  const bool is_admin = session_dao.is_admin(get_user_id());
  if (!is_admin) {
    response.content.clear();
    response.content.str("");
    response.status_code = HTTPStatus::forbidden;
    create_full_html_page_for_standard_response(response);
    return;
  }
  auto tile_report = session_dao.get_tile_usage_metrics(12);
  build_form(response.content, tile_report);
}
