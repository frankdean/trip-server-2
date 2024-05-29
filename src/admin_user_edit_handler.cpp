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
#include "admin_user_edit_handler.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/dao_helper.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/uuid.hpp"
#include <sstream>
#include <syslog.h>

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
  if (!new_user_flag && user_details.id.has_value()) {
    os << "      <input type=\"hidden\" name=\"user_id\" value=\"" << user_details.id.value() << "\">\n";
  }
  if (nickname_required_error) {
    os <<
      // Error message displayed when the nickname field is empty when editing a user record
      "    <div id=\"nickname-required-error\" class=\"m-0 alert alert-danger\" role=\"alert\">" << translate("Nickname is required") << "    </div>\n";
  }
  os <<
    "      <div class=\"py-1\">\n"
    // Label for the user's nickname field
    "        <label for=\"nickname\">" << translate("Nickname") << "</label>\n"
    "        <input id=\"nickname\" name=\"nickname\" value=\"" << x(user_details.nickname) << "\" required>\n"
    "      </div>\n";
  if (email_required_error) {
    os <<
      // Error message displayed when the email field is empty when editing a user record
      "    <div id=\"email-required-error\" class=\"m-0 alert alert-danger\" role=\"alert\">" << translate("Email is required") << "    </div>\n";
  }
  os <<
    "      <div class=\"py-1\">\n"
    // Label for the user's email field
    "        <label for=\"email\">" << translate("Email") << "</label>\n"
    "        <input id=\"email\" name=\"email\" value=\"" << x(user_details.email) << "\" required>\n"
    "      </div>\n";
  if (first_name_required_error) {
    os <<
      // Error message displayed when the first name field is empty when editing a user record
      "    <div id=\"first-name-required-error\" class=\"m-0 alert alert-danger\" role=\"alert\">" << translate("First name is required") << "    </div>\n";
  }
  os <<
    "      <div class=\"py-1\">\n"
    // Label for the user's firstname field
    "        <label for=\"firstname\">" << translate("First name") << "</label>\n"
    "        <input id=\"firstname\" name=\"firstname\" value=\"" << x(user_details.firstname) << "\" required>\n"
    "      </div>\n";
  if (last_name_required_error) {
    os <<
      // Error message displayed when the last name field is empty when editing a user record
      "    <div id=\"last-name-required-error\" class=\"m-0 alert alert-danger\" role=\"alert\">" << translate("Last name is required") << "    </div>\n";
  }
  os <<
    "      <div class=\"py-1\">\n"
    // Label for the user's lastname field
    "        <label for=\"lastname\">" << translate("Last name") << "</label>\n"
    "        <input id=\"lastname\" name=\"lastname\" value=\"" << x(user_details.lastname) << "\" required>\n"
    "      </div>\n";
  if (password_required_error) {
    os <<
      // Error message displayed when the password field is empty when editing a user record
      "    <div id=\"password-required-error\" class=\"m-0 alert alert-danger\" role=\"alert\">" << translate("Password is required") << "    </div>\n";
  }
  os <<
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
  user_details.id = request.get_optional_post_param_long("user_id");
  if (!user_details.id.has_value())
    user_details.uuid = UUID::generate_uuid();
  std::optional<std::string> v = request.get_optional_post_param("firstname");
  if (v)
    user_details.firstname = v.value();
  else
    first_name_required_error = true;
  v = request.get_optional_post_param("lastname");
  if (v)
    user_details.lastname = v.value();
  else
    last_name_required_error = true;
  v = request.get_optional_post_param("email");
  if (v)
    user_details.email = v.value();
  else
    email_required_error = true;
  v = request.get_optional_post_param("nickname");
  if (v)
    user_details.nickname = v.value();
  else
    nickname_required_error = true;

  user_details.password = request.get_optional_post_param("new-password");
  if (new_user_flag && !user_details.password.has_value())
    password_required_error = true;
    
  if (user_details.password.has_value()) {
    const auto confirm_password = request.get_post_param("confirm-password");
    passwords_match_failure = confirm_password != user_details.password;
  }

  const auto s = request.get_post_param("admin-role");
  user_details.is_admin = (s == "on");
  if (!(passwords_match_failure ||
        nickname_required_error ||
        email_required_error ||
        first_name_required_error ||
        last_name_required_error ||
        password_required_error)) {
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
    response.status_code = HTTPStatus::forbidden;
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
