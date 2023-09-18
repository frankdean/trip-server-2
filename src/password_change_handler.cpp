// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2023 Frank Dean <frank.dean@fdsd.co.uk>

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
#include "password_change_handler.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;

void PasswordChangeHandler::build_form(std::ostream &os)
{
  os <<
    "<div class=\"container-fluid bg-light my-3\">\n"
    "  <h1>" << get_page_title() << "</h1>\n";
  if (bad_credentials) {
    os << "    <div id=\"bad-credential\" class=\"alert alert-danger\" role=\"alert\">"
      // Warning message displayed when the current password is incorrect during password change
       << translate("Incorrect current password")
       << "    </div>\n";
  } else if (week_password) {
    os <<
      // Warning message displayed when a new password is too week
      "    <div id=\"week-password\" class=\"alert alert-warning\" role=\"alert\">" << translate("The new password is too week.  Try choosing a password with more characters.") <<
      "    </div>\n";
  } else if (passwords_match_failure) {
    os <<
      // Warning message displayed when the two new passwords do not match during password change
      "    <div id=\"password-match-failure\" class=\"alert alert-warning\" role=\"alert\">" << translate("New passwords do not match") <<
      "    </div>\n";
  }
  os <<
    "  <form method=\"post\">\n"
    "    <div>\n"
    "      <div class=\"mb-3\">\n"
    // Label prompting for current password during password change
    "        <label for=\"current-password\">" << translate("Current password") << "</label>\n"
    "        <input id=\"current-password\" type=\"password\" autocomplete=\"current-password\" name=\"password\" size=\"32\" maxLength=\"120\" required>\n"
    "      </div>\n"
    "      <div class=\"mb-3\">\n"
    "        <div class=\"input-hint alert alert-info mb-3\">\n"
    /// Advice on choosing a password
     << translate("Strong passwords should avoid real words, be at least 9 characters and use a mixture of uppercase, lowercase, numeric and symbolic characters.  The strongest passwords will usually be a minimum of 11 characters") << "\n"
    "        </div>\n"
    // Label prompting for new password during password change
    "        <label for=\"new-password\">" << translate("New password") << "</label>\n"
    "        <input id=\"new-password\" class=\"password-strength\" type=\"password\" autocomplete=\"new-password\" name=\"new-password\" size=\"32\" maxlength=\"120\" required>\n"
    "      </div>\n"
    "      <input id=\"pw-score\" type=\"hidden\" name=\"score\" value=\"0\">\n"
    "      <div id=\"pw-strength-wrapper\" class=\"progress\">\n"
    "        <div id=\"pw-strength\">\n"
//    "          <span id=\"pw-meter\" class=\"d-block\" role=\"progressBar\">&nbsp;</span>\n"
    // "          <span ng-show=\"passwordStrength == null || passwordStrength == 0\">&nbsp;</span>\n"
    // "          <span ng-show=\"passwordStrength == 1\">Very weak</span>\n"
    // "          <span ng-show=\"passwordStrength == 2\">Weak</span>\n"
    // "          <span ng-show=\"passwordStrength == 3\">Strong</span>\n"
    // "          <span ng-show=\"passwordStrength == 4\">Very strong</span>\n"
    "        </div>\n"
    "      </div>\n"
    "      <div id=\"crack-time-div\" class=\"d-none\">\n"
    // Displays estimate password crack time.  HTML span element is replaced with value at runtime.
    "        <p>" << translate("Estimated <span id=\"crack-time\"></span> to crack with ten attempts per second.") << "</p>\n"
    "      </div>\n"
    "      <div id=\"feedback-warning\" class=\"d-none\"></div>\n"
    "      <div id=\"feedback-suggestions\" class=\"d-none\"></div>\n"
    "      <div>\n"
    // Label prompting to repeat the new password during password change
    "        <label for=\"confirm-password\">" << translate("Repeat password") << "</label>\n"
    "        <input id=\"confirm-password\" type=\"password\" autocomplete=\"new-password\" name=\"confirm-password\" size=\"32\" maxlength=\"120\" required>\n"
    "      </div>\n"
    "    </div>\n"
    "    <div class=\"py-3\">\n"
    // Label to save password change
    "      <button id=\"btn-save\" class=\"btn btn-lg btn-success\">" << translate("Save") << "</button>\n"
    // Label to cancel changing password
    "      <button id=\"btn-cancel\" class=\"btn btn-lg btn-danger\" name=\"action\" value=\"cancel\" formnovalidate>" << translate("Cancel") << "</button>\n"
    "    </div>\n"
    "  </form>\n"
    "</div>\n";
}

void PasswordChangeHandler::append_pre_body_end(std::ostream& os) const
{
  TripAuthenticatedRequestHandler::append_pre_body_end(os);
  os << "    <script type=\"text/javascript\" src=\"" << get_uri_prefix() << "/static/zxcvbn-" << ZXCVBN_VERSION << "/dist/zxcvbn.js\"></script>\n";
  os << "    <script type=\"text/javascript\" src=\"" << get_uri_prefix() << "/static/js/change-password.js\"></script>\n";
}

void PasswordChangeHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  set_menu_item(unknown);
  // Title of the Password Change page
  set_page_title(translate("Change Password"));
}

void PasswordChangeHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  const std::string action = request.get_post_param("action");
  if (action == "cancel") {
    redirect(request, response, get_uri_prefix() + "/account");
    return;
  }
  if (request.method == HTTPMethod::post) {
    const std::string password = request.get_post_param("password");
    const std::string new_password = request.get_post_param("new-password");
    const std::string confirm_password = request.get_post_param("confirm-password");
    const int score = std::stoi(request.get_post_param("score"));
    if (new_password != confirm_password) {
      passwords_match_failure = true;
    } else if (score < 3) {
      week_password = true;
    } else {
      SessionPgDao dao;
      if (dao.validate_password_by_user_id(get_user_id(), password)) {
        dao.change_password(get_user_id(), new_password);
        redirect(request, response, get_uri_prefix() + "/account");
      } else {
        bad_credentials = true;
      }
    }
  }
  build_form(response.content);
}
