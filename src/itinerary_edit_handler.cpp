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
#include "itinerary_edit_handler.hpp"
#include "../trip-server-common/src/dao_helper.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

void ItineraryEditHandler::build_form(
    web::HTTPServerResponse& response,
    const ItineraryPgDao::itinerary_description& itinerary)
{
  response.content
    <<
    "            <div>\n"
    // Title of the itinerary page when it is being edited
    "              <h1>" << translate("Edit Itinerary") << "</h1>\n";
  if (no_title_error)
    response.content
      <<
      "            <div class=\"alert alert-danger\">\n"
      // Message displayed when no value has been input for the mandatory title
      "              <p>" << translate("You must enter a title") << "</p>\n"
      "            </div>\n";
  response.content
    <<
    "              <form method=\"post\">\n";
  if (itinerary_id.first) {
    response.content
      <<
      "                <input type=\"hidden\" name=\"id\" value=\"" << itinerary_id.second << "\">\n";
  }
  response.content
    <<
    "                <div class=\"container-fluid bg-light row g-3 my-3\">\n"
    "                  <div class=\"col-lg-6\">\n"
    // Label for entering the itinerary title
    "                    <label for=\"input-title\" class=\"form-label\">" << translate("Title") << "</label>\n"
    "                    <input id=\"input-title\" class=\"form-control\" type=\"text\" name=\"title\" value=\"" << x(itinerary.title) << "\" required>\n"
    "                  </div>\n"
    "                  <div class=\"col-lg-3\">\n"
    // Label for entering the start date for an itinerary
    "                    <label for=\"input-date-from\" class=\"form-label\">" << translate("Date from") << "</label>\n"
    "                    <input id=\"input-date-from\" class=\"form-control\" type=\"date\" name=\"from\" value=\"";
  if (itinerary.start.first)
    response.content << dao_helper::date_as_html_input_value(itinerary.start.second);
  response.content
    <<
    "\">\n"
    "                  </div>\n"
    "                  <div class=\"col-lg-3\">\n"
    // Label for entering the end date for an itinerary
    "                    <label for=\"input-date-to\" class=\"form-label\">" << translate("Date to") << "</label>\n"
    "                    <input id=\"input-date-to\" class=\"form-control\" type=\"date\" name=\"to\" value=\"";
  if (itinerary.finish.first)
    response.content << dao_helper::date_as_html_input_value(itinerary.finish.second);
  response.content
    <<
    "\">\n"
    "                  </div>\n"
    "                  <div class=\"col-12\">\n"
    // Title for the itinerary description field when being edited
    "                    <label for=\"raw-textarea\" class=\"form-label\">" << translate("Description") << "</label>\n"
    "                    <textarea id=\"raw-textarea\" name=\"description\" class=\"raw-markdown\" rows=\"12\">";
  if (itinerary.description.first)
    response.content << itinerary.description.second;
  response.content
    <<
    "</textarea>\n"
    "                  </div>\n"
    "                </div>\n"
    "                <div class=\"col-12 pt-3\" arial-label=\"Form buttons\">\n"
    // Label for the save itinerary button
    "                  <button type=\"submit\" class=\"py-3 btn btn-lg btn-success\" name=\"action\" value=\"save\" accesskey=\"s\">" << translate("Save") << "</button>\n";
  if (!is_new) {
    response.content
      <<
      // Confirmation dialog text when deleting an itinerary
      "                  <button type=\"submit\" class=\"py-3 btn btn-lg btn-danger\" name=\"action\" value=\"delete\" accesskey=\"d\" onclick=\"return confirm('" << translate("Delete this itinerary and ALL uploaded GPX items?") << "');\">"
      // Label for the delete itinerary button
      << translate("Delete") << "</button>";
  }
  response.content
    <<
    // Confirmation dialog text when cancelling editing an itinerary
    "                  <button type=\"submit\" class=\"py-3 btn btn-lg btn-danger\" name=\"action\" value=\"cancel\" accesskey=\"c\" formnovalidate onclick=\"return confirm('"
    // Text to confirm when cancelling editing an itinerary
    << translate("Cancel?") << "'); \">"
    // Button label to cancel editing and itinerary description
    << translate("Cancel") << "</button>\n"
    // Confirmation dialog text when performing a form reset whilst editing an itinerary
    "                  <button type=\"reset\" class=\"py-3 btn btn-lg btn-danger\" name=\"action\" value=\"reset\" onclick=\"return confirm('" << translate("Reset form to original state, losing all your changes") << "'); \">"
    // Form reset label
    << translate("Reset") << "</button>\n"
    "                </div>\n"
    "              </form>\n"
    "            </div>\n";
}

void ItineraryEditHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  const std::string id  = request.get_param("id");
  is_new = id.empty();
  if (is_new) {
    // Title for the page when creating and editing a new itinerary
    set_page_title(translate("Itinerary Description&mdash;New"));
  } else {
    // Title for the page when editing an existing itinerary
    set_page_title(translate("Itinerary Description&mdash;Edit"));
    itinerary_id = std::make_pair(true, std::stol(id));
  }
  // set_menu_item(unknown);
}

void ItineraryEditHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  // std::cout << request.content << '\n';
  const std::string action=request.get_post_param("action");
  // std::cout << "action: \"" << action << "\"\n";
  if (action == "save") {
    ItineraryPgDao dao;
    ItineraryPgDao::itinerary_description itinerary;
    itinerary.id = itinerary_id;
    std::string s = request.get_post_param("from");
    if ((itinerary.start.first = !s.empty())) {
      DateTime start(s);
      itinerary.start.second = start.time_tp();
    }
    s = request.get_post_param("to");
    itinerary.finish.first = !s.empty();
    if (itinerary.finish.first) {
      DateTime finish(s);
      itinerary.finish.second = finish.time_tp();
    }
    // Swap the dates if the start date is after the finish date
    if (itinerary.start.first && itinerary.finish.first &&
        itinerary.finish.second < itinerary.start.second) {
      const auto hold = itinerary.start;
      itinerary.start = itinerary.finish;
      itinerary.finish = hold;
    }
    itinerary.title = request.get_post_param("title");
    dao_helper::trim(itinerary.title);
    no_title_error = itinerary.title.empty();
    s = request.get_post_param("description");
    if ((itinerary.description.first = !s.empty())) {
      itinerary.description.second = s;
    }
    if (!no_title_error) {
      long id = dao.save_itinerary(get_user_id(), itinerary);
      // Set the itinerary ID after creating a new one
      if (!itinerary_id.first)
        itinerary_id = std::make_pair(true, id);
    } else {
      build_form(response, itinerary);
      return;
    }
  } else if (action == "delete") {
    if (itinerary_id.first) {
      ItineraryPgDao dao;
      dao.delete_itinerary(get_user_id(), itinerary_id.second);
      redirect(request, response,
               get_uri_prefix() + "/itineraries");
      return;
    }
  } else if (action == "cancel") {
    // drop through to redirect based on whether itinerary_id is set
  } else {
    if (!is_new) {
      ItineraryPgDao dao;
      auto itinerary = dao.get_itinerary_description(get_user_id(), itinerary_id.second);
      if (!itinerary.first)
        throw BadRequestException("Itinerary ID not found");
      build_form(response, itinerary.second);
      return;
    } else {
      ItineraryPgDao::itinerary_description itinerary;
      build_form(response, itinerary);
      return;
    }
  }
  if (itinerary_id.first) {
    redirect(request, response,
             get_uri_prefix() + "/itinerary?id=" + std::to_string(itinerary_id.second));
  } else {
    redirect(request, response,
             get_uri_prefix() + "/itineraries");
  }
  return;
}
