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
#include "itinerary_sharing_handler.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/pagination.hpp"
#include <boost/locale.hpp>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

void ItinerarySharingHandler::build_form(HTTPServerResponse& response,
                const Pagination& pagination,
                const std::vector<ItineraryPgDao::itinerary_share>& itinerary_shares) const
{
  response.content
    <<
    "<div id=\"itinerary-shares\" class=\"container-fluid\">\n"
    "  <h1 class=\"pt-2\">" << translate("Itinerary Sharing") << "</h1>\n";
  response.content
    <<
    "  <form name=\"form\" method=\"post\">\n"
    "          <input type=\"hidden\" name=\"itinerary_id\" value=\"" << itinerary_id << "\">\n"
    "          <input type=\"hidden\" name=\"routing\" value=\"" << routing << "\">\n"
    "          <input type=\"hidden\" name=\"report-page\" value=\"" << report_page << "\">\n";
  if (itinerary_shares.empty()) {
    response.content
      << "<div class=\"alert alert-info\"><p>"
      // Message displayed when there are no itinerary sharing records to show
      << translate("You are not currently sharing your itinerary with anyone")
      << "</p></div>\n";
  } else {
    response.content
      <<
      "    <div class=\"table-responsive\">\n"
      "      <table id=\"table-shares\" class=\"table table-striped\">\n"
      "        <tr>\n"
      // Nickname column heading in itinerary sharing list
      "          <th>" << translate("Nickname") << "</th>\n"
      // Column heading indicating sharing is active in itinerary sharing list
      "          <th>" << translate("Active") << "</th>\n"
      "          <th><input id=\"select-all\" accesskey=\"a\" type=\"checkbox\" class=\"form-check-input\" onclick=\"select_all(this)\"></th>\n"
      "        </tr>\n";
    for (const auto &share : itinerary_shares) {
      response.content
        <<
        "        <tr>\n"
        "          <td>" << x(share.nickname) << "</td>\n"
        "          <td>" << (share.active.has_value() && share.active.value() ? "&#x2713;" : "") << "</td>\n"
        "          <td><input type=\"checkbox\" name=\"shared_to_id[" << share.shared_to_id << "]\"></td>\n"
        "        </tr>\n";
    }
    response.content
      <<
      "      </table>\n"
      "    </div>\n";
    const auto page_count = pagination.get_page_count();
    if (page_count > 1) {
      response.content
        <<
        "    <div id=\"div-paging\" class=\"pb-0\">\n"
        << pagination.get_html()
        <<
        "    </div>\n"
        "    <div class=\"d-flex justify-content-center pt-0 pb-0 col-12\">\n"
        "      <input id=\"goto-page\" type=\"number\" name=\"goto-page\" value=\""
        << std::fixed << std::setprecision(0) << pagination.get_current_page()
        << "\" min=\"1\" max=\"" << page_count << "\">\n"
        // Title of button which goes to a specified page number
        "      <button id=\"goto-page-btn\" class=\"btn btn-sm btn-primary\" type=\"submit\" name=\"action\" accesskey=\"g\" value=\"goto-page\">" << translate("Go") << "</button>\n"
        "    </div>\n"
        ;
    }
  }
  response.content
    <<
    "    <div id=\"div-buttons\">\n";
  if (!itinerary_shares.empty()) {
    response.content
      <<
      // Label for button to activate an itinerary share
      "      <button id=\"btn-activate\" name=\"action\" value=\"activate\" accesskey=\"v\" class=\"my-1 btn btn-lg btn-success\">" << translate("Activate") << "</button>\n"
      // Label for button to deactivate an itinerary share
      "      <button id=\"btn-deactivate\" name=\"action\" value=\"deactivate\" accesskey=\"x\" class=\"my-1 btn btn-lg btn-primary\">" << translate("Deactivate") << "</button>\n"
      // Label for button to delete selected list of itinerary shares
      "      <button id=\"btn-delete\" name=\"action\" value=\"delete\" accesskey=\"d\" class=\"my-1 btn btn-lg btn-danger\" onclick=\"return confirm('" << translate("Delete selected nicknames?") << "');\">" << translate("Delete selected") << "</button>\n";
  }
  response.content
    <<
    // Label for button to create a new itinerary share
    "      <button id=\"btn-new\" accesskey=\"w\" formmethod=\"get\" formaction=\"" << get_uri_prefix() << "/itinerary-sharing/edit?itinerary_id=" << itinerary_id << "\" class=\"my-1 btn btn-lg btn-warning\">" << translate("New") << "</button>\n";
  if (!routing.empty()) {
    response.content <<
      // Label for button to return to the itinerary when viewing a list of itinerary shares
      "      <button id=\"btn-close\" accesskey=\"i\" formmethod=\"get\" formaction=\"" << get_uri_prefix() << "/itinerary\" class=\"my-1 btn btn-lg btn-danger\">" << translate("Itinerary") << "</button>\n";
  }
  response.content <<
    // Label for button to return to the itinerary or itinerary sharing report when viewing a list of itinerary shares
    "      <button id=\"btn-close\" accesskey=\"c\" formmethod=\"get\" formaction=\"" << get_uri_prefix();
  if (!routing.empty()) {
    response.content << "/" << routing << "?report-page=" << report_page << "\"";
  } else {
    response.content << "/itinerary\"";
  }
  response.content <<
    " class=\"my-1 btn btn-lg btn-danger\">" << translate("Close") << "</button>\n"
    "    </div>\n"
    "  </form>\n";
  response.content
    <<
    "</div>\n"
    "<script>\n"
    "<!--\n"
    "function select_all(cb) {\n"
    "  const div = document.getElementById('itinerary-shares');\n"
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

void ItinerarySharingHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  (void)response; // unused
  itinerary_id = std::stol(request.get_query_param("id"));
  set_page_title(translate("Itinerary Sharing"));
  // set_menu_item(unknown);
  routing = request.get_param("routing");
  report_page = request.get_param("report-page");
}

void ItinerarySharingHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  ItineraryPgDao dao;
  if (request.method == HTTPMethod::post) {
    const std::string action = request.get_post_param("action");
    std::map<long, std::string> sharing_map =
      request.extract_array_param_map("shared_to_id");
    std::vector<long>shared_to_ids;
    for (const auto &m : sharing_map) {
      if (m.second == "on") {
        // std::cout << "shared ids " << m.first << " -> " << m.second << '\n';
        shared_to_ids.push_back(m.first);
      }
    }
    if (!shared_to_ids.empty()) {
      if (action == "activate") {
        dao.activate_itinerary_shares(get_user_id(),
                                      itinerary_id,
                                      shared_to_ids,
                                      true);
      } else if (action == "deactivate") {
        dao.activate_itinerary_shares(get_user_id(),
                                      itinerary_id,
                                      shared_to_ids,
                                      false);
      } else if (action == "delete") {
        dao.delete_itinerary_shares(get_user_id(),
                                    itinerary_id,
                                    shared_to_ids);
      }
    }
  }
  std::map<std::string, std::string> page_param_map;
  page_param_map["id"] = std::to_string(itinerary_id);
  const long total_count = dao.get_itinerary_shares_count(get_user_id(),
                                                          itinerary_id);
  // std::cout << "Got " << total_count << " itinerary shares for user ID " << get_user_id() << " and itinerary ID " << itinerary_id << "\n";
  // if (total_count == 0) {
  //   redirect(request,
  //            response,
  //            get_uri_prefix() + "/itinerary-sharing/edit?itinerary_id=" +
  //            std::to_string(itinerary_id));
  //   return;
  // }
  Pagination pagination(get_uri_prefix() + "/itinerary-sharing",
                        page_param_map,
                        total_count,
                        10,
                        5,
                        true,
                        true,
                        "goto-page");
  const std::string page = request.get_param("goto-page");
  try {
    if (!page.empty())
      pagination.set_current_page(std::stoul(page));
  } catch (const std::logic_error& e) {
    std::cerr << "Error converting string to page number\n";
  }
  auto itinerary_shares = dao.get_itinerary_shares(get_user_id(),
                                                   itinerary_id,
                                                   pagination.get_offset(),
                                                   pagination.get_limit());
  build_form(response, pagination, itinerary_shares);
}
