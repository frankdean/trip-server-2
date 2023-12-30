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
#include "itinerary_sharing_edit_handler.hpp"
#include "tracking_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <sstream>
#include <boost/locale.hpp>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

void ItinerarySharingEditHandler::build_form(
    HTTPServerResponse& response,
    const ItineraryPgDao::itinerary_share &itinerary_share) const
{
  response.content
    <<
    "<div id=\"itinerary-sharing-edit-form\" class=\"container-fluid\">\n";
  if (invalid_nickname_error)
    response.content
      <<
      "            <div class=\"alert alert-danger\" role=\"alert\">\n"
      // Message displayed when an entered nickname does not exist
      "              <p>" << translate("That nickname does not exist.  Nicknames are case-sensitive.") << "</p>\n"
      "            </div>\n";
  response.content
    <<
    "  <form name=\"form\" method=\"post\">\n"
    "    <div class=\"container-fluid bg-light row g-3 my-3 pb-3 mx-0\">\n"
    "      <input type=\"hidden\" name=\"itinerary_id\" value=\"" << itinerary_id << "\">\n"
    "      <input type=\"hidden\" name=\"routing\" value=\"" << routing << "\">\n"
    "      <input type=\"hidden\" name=\"report-page\" value=\"" << report_page << "\">\n"
    "      <input type=\"hidden\" name=\"goto-page\" value=\"" << current_page << "\">\n";
  if (shared_to_id.first)
    response.content
      <<
      "      <input type=\"hidden\" name=\"shared_to_id\" value=\"" << shared_to_id.second << "\">\n";
  if (!is_new) {
    // If editing an existing user, the nickname is not submitted for the
    // disabled field, so include a hidden value to be posted
    response.content
      <<
      "      <input type=\"hidden\" name=\"nickname\" value=\"" << x(itinerary_share.nickname) << "\">";
  }
  response.content
    <<
    // Label for input of nickname when sharing an itinerary
    "      <label for=\"input-nickname\">" << translate("Nickname") << "</label>\n"
    "      <input id=\"input-nickname\" name=\"nickname\" value=\"" << x(itinerary_share.nickname) << "\"";
  if (!is_new) {
    response.content << " disabled";
  } else {
    response.content << " required";
  }
  response.content
    <<
    " autofocus>\n"
    "    </div>\n"
    "    <div id=\"div-active\">\n"
    // Label for checkbox indicating whether itinerary sharing is active for a specific nickname
    "      <label for=\"input-active\">" << translate("Active") << "</label>\n"
    "      <input id=\"input-active\" type=\"checkbox\" name=\"active\"";
  if (itinerary_share.active.first && itinerary_share.active.second)
    response.content << " checked";
  response.content
    <<
    ">\n"
    "    </div>\n"
    "    <div id=\"div-form-buttons\">\n"
    "      <button id=\"btn-save\" accesskey=\"s\" class=\"my-1 btn btn-lg btn-primary\" name=\"action\" value=\"save\">" << translate("Save") << "</button>\n"
    "      <button id=\"btn-cancel\" class=\"my-1 btn btn-lg btn-danger\" name=\"action\" value=\"cancel\" formnovalidate>"
    // Label of button to cancel editing or creating an individual itinerary share
    << translate("Cancel") << "</button>\n"
    "    </div>\n"
    "  </form>\n"
    "</div>\n";
}

void ItinerarySharingEditHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  itinerary_id = std::stol(request.get_param("itinerary_id"));
  const std::string s = request.get_param("shared_to_id");
  shared_to_id.first = !s.empty();
  if (shared_to_id.first) {
    shared_to_id.second = std::stol(s);
  }
  is_new = !shared_to_id.first;
  if (is_new) {
    // Title for the page when creating an itinerary share
    set_page_title(translate("Share Itinerary&mdash;New"));
  } else {
    // Title for the page when editing an existing itinerary share
    set_page_title(translate("Share Itinerary&mdash;New"));
  }
  routing = request.get_param("routing");
  report_page = request.get_param("report-page");
}

void ItinerarySharingEditHandler::handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response)
{
  const std::string page = request.get_param("goto-page");
  try {
    if (!page.empty())
      current_page = std::stoul(page);
  } catch (const std::logic_error& e) {
    std::cerr << "Error converting string to page number\n";
  }
  ItineraryPgDao::itinerary_share itinerary_share;
  if (request.method == HTTPMethod::post) {
    const std::string s = request.get_post_param("active");
    if (!s.empty() && s == "on") {
      itinerary_share.active.first = true;
      itinerary_share.active.second = true;
    }
    const std::string action = request.get_post_param("action");
    if (action == "save") {
      itinerary_share.nickname = request.get_post_param("nickname");
      dao_helper::trim(itinerary_share.nickname);
      if (!itinerary_share.nickname.empty()) {
        TrackPgDao tracking_dao(elevation_service);
        try {
          const std::string shared_to_id_str = tracking_dao.get_user_id_by_nickname(itinerary_share.nickname);
          itinerary_share.shared_to_id = std::stol(shared_to_id_str);
        } catch (const std::out_of_range &e) {
          invalid_nickname_error = true;
        }
      } else {
        invalid_nickname_error = true;
      }
      if (!invalid_nickname_error) {
        ItineraryPgDao dao;
        // std::cout << "Saving itinerary share for nickname \"" << itinerary_share.nickname << "\"\n";
        dao.save(get_user_id(), itinerary_id, itinerary_share);
        std::ostringstream os;
        os << get_uri_prefix()
           << "/itinerary-sharing?id=" << itinerary_id
           << "&goto-page=" << current_page
           << "&routing=" << routing
           << "&report-page=" << report_page;
        redirect(request, response, os.str());
        return;
      }
    } else if (action == "cancel") {
      std::ostringstream os;
      os << get_uri_prefix() << "/itinerary-sharing?id=" << itinerary_id << "&goto-page=" << current_page;
      redirect(request, response, os.str());
      return;
    }
  } else if (!is_new && shared_to_id.first) {
    ItineraryPgDao dao;
    itinerary_share = dao.get_itinerary_share(get_user_id(),
                                              itinerary_id,
                                              shared_to_id.second);
  }
  build_form(response, itinerary_share);
}
