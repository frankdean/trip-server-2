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
#include "admin_user_edit_handler.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/dao_helper.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/uuid.hpp"

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;

void AdminUserEditHandler::build_form(
    std::ostream &os,
    SessionPgDao::user user_details)
{
  os <<
    "<div class=\"container-fluid my-3\">\n"
    "  <h1>" << get_page_title() << "</h1>\n";
  if (user_not_found) {
    os <<
      "  <div id=\"no-users-found\" class=\"alert alert-info\">\n"
      // Message show when a user cannot be found for edit
      "    <p>" << translate("User not found") << "</p>\n"
      "  </div>\n";
  }
  os <<
    "  <div class=\"container-fluid bg-light\">\n"
    "    <form name=\"form\" method=\"post\">\n";
  if (!new_user_flag && user_details.id.first) {
    os << "      <input type=\"hidden\" name=\"user_id\" value=\"" << user_details.id.second << "\">\n";
  }
  os <<
    "      <div class=\"py-1\">\n"
    // Label for the user's nickname field
    "        <label for=\"nickname\">" << translate("Nickname") << "</label>\n"
    "        <input id=\"nickname\" name=\"nickname\" value=\"" << x(user_details.nickname) << "\" required>\n"
    "      </div>\n"
    "      <div class=\"py-1\">\n"
    // Label for the user's email field
    "        <label for=\"email\">" << translate("Email") << "</label>\n"
    "        <input id=\"email\" name=\"email\" value=\"" << x(user_details.email) << "\" required>\n"
    "      </div>\n"
    "      <div class=\"py-1\">\n"
    // Label for the user's firstname field
    "        <label for=\"firstname\">" << translate("First name") << "</label>\n"
    "        <input id=\"firstname\" name=\"firstname\" value=\"" << x(user_details.firstname) << "\" required>\n"
    "      </div>\n"
    "      <div class=\"py-1\">\n"
    // Label for the user's lastname field
    "        <label for=\"lastname\">" << translate("Last name") << "</label>\n"
    "        <input id=\"lastname\" name=\"lastname\" value=\"" << x(user_details.lastname) << "\">\n"
    "      </div>\n"
    "      <div id=\"div-password\" class=\"py-1\">\n";
  if (!new_user_flag) {
    // Instructions on using change password fields
    os << "        <div>" << translate("Leave password fields blank, unless changing password.") << "</div>\n";
  }
  os <<
    "        <div class=\"py-1\">\n"
    // Label for input new password for a user
    "          <label for=\"new-password\">" << translate("Password") << "</label>\n"
    "          <input id=\"new-password\" type=\"password\" autocomplete=\"off\" name=\"new-password\" size=\"32\" maxlength=\"120\"";
  if (new_user_flag)
    os << " required";
  os << ">\n"
    "        </div>\n"
    "        <div class=\"py-1\">\n"
    // Label prompting to repeat the new password during password change
    "          <label for=\"confirm-password\">" << translate("Repeat password") << "</label>\n"
    "          <input id=\"confirm-password\" type=\"password\" autocomplete=\"off\" name=\"confirm-password\" size=\"32\" maxlength=\"120\" ";
  if (new_user_flag)
    os << " required";
  os << ">\n"
    "        </div>\n";
  if (passwords_match_failure) {
    os <<
      // Warning message displayed when the two new passwords do not match during password change
      "    <div id=\"password-match-failure\" class=\"alert alert-warning\" role=\"alert\">" << translate("New passwords do not match") << "    </div>\n";
  }
  os <<
    "      </div>\n"
    "      <div class=\"py-1\">\n"
    // Label for the user's administration role checkbox
    "        <label for=\"admin-role\">" << translate("Administrator") << "</label>\n"
    "        <input id=\"admin-role\" type=\"checkbox\" name=\"admin-role\"";
  if (user_details.is_admin)
    os << " checked";
  os << ">\n"
    "      </div>\n";
  if (save_failed_flag) {
    // Error message shown when saving a new user fails, probably due to non-unique email or nickname
    os << "<div class=\"alert alert-danger\">" << translate("Failed to save user.  Make sure that the email and nickname values are unique.") << "</div>\n";
  }
  os <<
    "      <div class=\"py-1\">\n"
    // Label to save user details
    "        <button id=\"btn-save\" class=\"btn btn-lg btn-success\" name=\"action\" value=\"save\">" << translate("Save") << "</button>\n"
    // Label to cancel saving user details
    "        <button id=\"btn-cancel\" class=\"btn btn-lg btn-danger\" name=\"action\" value=\"cancel\" formnovalidate>" << translate("Cancel") << "</button>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n";
}

void AdminUserEditHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  user_id = request.get_param("id");
  new_user_flag = user_id.empty();
  // Page title of the admin user's User Management page
  set_page_title(translate("User Edit"));
  set_menu_item(users);
}

void AdminUserEditHandler::save(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response,
    SessionPgDao &session_dao)
{
  SessionPgDao::user user_details;
  try {
    user_details.id.second = std::stol(request.get_post_param("user_id"));
    user_details.id.first = true;
  } catch (const std::invalid_argument &e) {
    user_details.id.first = false;
    user_details.uuid = std::make_pair(true, UUID::generate_uuid());
  }
  user_details.firstname = request.get_post_param("firstname");
  user_details.lastname = request.get_post_param("lastname");
  user_details.email = request.get_post_param("email");
  user_details.nickname = request.get_post_param("nickname");
  user_details.password.second = request.get_post_param("new-password");
  user_details.password.first = !user_details.password.second.empty();
  dao_helper::trim(user_details.firstname);
  dao_helper::trim(user_details.lastname);
  dao_helper::trim(user_details.email);
  dao_helper::trim(user_details.nickname);

  if (user_details.password.first) {
    const auto confirm_password = request.get_post_param("confirm-password");
    passwords_match_failure = confirm_password != user_details.password.second;
  }
  const auto s = request.get_post_param("admin-role");
  user_details.is_admin = (s == "on");
  if (!passwords_match_failure) {
    try {
      session_dao.save(user_details);
      redirect(request, response, get_uri_prefix() + "/users");
      return;
    } catch (const std::exception &e) {
      std::cerr << "Failure saving user details: "
                << e.what() << '\n';
      save_failed_flag = true;
      build_form(response.content, user_details);
      return;
    }
  } else {
    build_form(response.content, user_details);
    return;
  }
}

void AdminUserEditHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  SessionPgDao session_dao;
  const bool is_admin = session_dao.is_admin(get_user_id());
  if (!is_admin) {
    response.content.clear();
    response.content.str("");
    response.status_code = HTTPStatus::unauthorized;
    create_full_html_page_for_standard_response(response);
    return;
  }
  const auto action = request.get_post_param("action");
  if (action == "cancel") {
    redirect(request, response, get_uri_prefix() + "/users");
    return;
  } else if (action == "save") {
    save(request, response, session_dao);
    return;
  }
  if (!new_user_flag) {
    auto user_details = session_dao.get_user_details_by_user_id(user_id);
    build_form(response.content, user_details);
  } else {
    SessionPgDao::user user_details;
    build_form(response.content, user_details);
  }
}
