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
#include "admin_user_management_handler.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/uri_utils.hpp"

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;

void AdminUserManagementHandler::build_form(
    std::ostream &os,
    Pagination &pagination,
    std::vector<fdsd::trip::SessionPgDao::user> users)
{
  os <<
    "<div class=\"container-fluid my-3\">\n"
    "  <h1>" << get_page_title() << "</h1>\n";
  if (false) {
    os <<
      "  <div id=\"save-failed\" class=\"alert alert-danger\" role=\"alert\">"
      // Error message displayed when failure saving a user's details
       << translate("Save failed.  The most likely cause is that the nickname or e-mail address already exist.") <<
      "  </div>\n";
  }
  os <<
    "  <form name=\"search\" method=\"post\">\n"
    "    <div id=\"search\">\n"
    // "      <div class=\"mb-3\">\n"
    // // Radio button grouping label for user search by either nickname or email
    // "        " << translate("Search by") << "&nbsp;\n"
    // "        <input id=\"radio-nickname\" type=\"radio\" name=\"search-by\" value=\"nickname\" >\n"
    // // Label for user search by nickname
    // "        <label for=\"radio-nickname\">&nbsp;" << translate("Nickname") << "&nbsp;</label>\n"
    // "        <input id=\"radio-email\" type=\"radio\" name=\"search-by\" value=\"email\" >\n"
    // // Label for user search by email
    // "        <label for=\"radio-email\">&nbsp;" << translate("Email") << "&nbsp;</label>\n"
    // "      </div>\n"
    "      <div class=\"mb-3\">\n"
    // Label prompting input of nickname for user search
    "        <label for=\"input-search-nickname\">" << translate("Nickname") << "</label>\n"
    "        <input id=\"input-search-nickname\" type=\"text\" name=\"search-nickname\" value=\"" << x(nickname) << "\" size=\"32\" maxlength=\"120\">\n"
    "      </div>\n"
    "      <div class=\"mb-3\">\n"
    // Label prompting input of email for user search
    "        <label for=\"input-search-email\">" << translate("Email address") << "</label>\n"
    "        <input id=\"input-search-email\" type=\"text\" name=\"search-email\" value=\"" << x(email) << "\" size=\"32\" maxlength=\"120\">\n"
    "      </div>\n"
    "      <div class=\"mb-3\">\n"
    // Radio button grouping label for user search by either exact or partial match
    "        " << translate("Search type") << "&nbsp;\n"
    "        <input id=\"radio-exact-match\" type=\"radio\" name=\"search-type\" value=\"exact\"" << (search_type == SessionPgDao::exact ? " checked" : "") << ">\n"
    // Radio button label for exact match when searching users
    "        <label for=\"radio-exact-match\">&nbsp;" << translate("Exact") << "&nbsp;</label>\n"
    "        <input id=\"radio-partial-match\" type=\"radio\" name=\"search-type\" value=\"partial\"" << (search_type == SessionPgDao::partial ? " checked" : "") << ">\n"
    // Radio button label for partial match when searching users
    "        <label for=\"radio-partial-match\">&nbsp;" << translate("Partial match") << "</label>\n"
    "      </div>\n"
    "      <div>\n"
    // Label for button used for user search
    "        <button id=\"btn-search\" class=\"btn btn-lg btn-primary mb-3\">" << translate("Search") << "</button>\n"
    "      </div>\n"
    "    </div>\n";
  if (empty_search_result) {
    os <<
      "  <div id=\"no-users-found\" class=\"alert alert-info\">\n"
      // Message shown when user search yields no results
      "    <p>" << translate("No matching users found") << "</p>\n"
      "  </div>\n";
  }
  if (delete_users_failed_flag) {
    os <<
      "  <div class=\"alert alert-danger\">\n"
      // Message shown when deleting one or more users failed, probably due to existence of associated data
      "    <p>" << translate("Failed to delete users, probably because there is data associated with one of the users, e.g. itineraries or recorded locations.") << "</p>\n"
      "  </div>\n";
  }
  if (!users.empty()) {
    os <<
      "    <div id=\"users\">\n"
      "      <table id=\"table-users\" class=\"table table-striped\">\n"
      "        <tr>\n"
      // Column heading for nickname in list of users
      "          <th>" << translate("Nickname") << "</th>\n"
      // Column heading for email in list of users
      "          <th>" << translate("Email") << "</th>\n"
      // Column heading for firstname in list of users
      "          <th>" << translate("First Name") << "</th>\n"
      // Column heading for lastname in list of users
      "          <th>" << translate("Last Name") << "</th>\n"
      // Column heading for UUID in list of users
      "          <th>" << translate("uuid") << "</th>\n"
      // Column heading indicating whether a user has the admin role in list of users
      "          <th>" << translate("Admin") << "</th>\n"
      "          <th></th>\n"
      "        </tr>\n";
    for (const auto user : users) {
      if (user.id.first) {
        os <<
          "        <tr>\n"
          "          <td><a href=\"" << get_uri_prefix() << "/edit-user?id=" << user.id.second << "\">" << x(user.nickname) << "</a></td>\n"
          "          <td>" << x(user.email) << "</td>\n"
          "          <td>" << x(user.firstname) << "</td>\n"
          "          <td>" << x(user.lastname) << "</td>\n"
          "          <td>";
        if (user.uuid.first)
          os << user.uuid.second;
        os << "</td>\n"
          "          <td>" << (user.is_admin == true ? "&#x2713;" : "") << "</td>\n"
          "          <td><input type=\"checkbox\" name=\"user-id[" << user.id.second << "]\" value=\"" << user.id.second << "\"></td>\n"
          "        </tr>\n";
      }
    }
    os <<
      "      </table>\n"
      "    </div>\n";
    
    const auto page_count = pagination.get_page_count();
    if (page_count > 1) {
      os
        <<
        "      <div id=\"div-paging\" class=\"pb-0\">\n"
        << pagination.get_html()
        <<
        "      </div>\n"
        "      <div class=\"d-flex justify-content-center pt-0 pb-0 col-12\">\n"
        "        <input id=\"page\" type=\"number\" name=\"page\" value=\""
        << std::fixed << std::setprecision(0) << pagination.get_current_page()
        << "\" min=\"1\" max=\"" << page_count << "\">\n"
        // Title of button which goes to a specified page number
        "        <button id=\"goto-page-btn\" class=\"btn btn-sm btn-primary\" name=\"action\" accesskey=\"g\" value=\"page\">" << translate("Go") << "</button>\n"
        "      </div>\n"
        ;
    }
    os <<
      "    <div id=\"div-choice-buttons\">\n"
      // Confirmation to delete a list of selected users
      "      <button id=\"btn-delete\" class=\"btn btn-lg btn-danger\" name=\"action\" value=\"delete-selected\" onclick=\"return confirm('" << translate("Delete the selected users?") << "');\">"
      // Label for button to delete a list of selected users
       << translate("Delete") << "</button>\n"
      // Label for button to edit a selected user
      "      <button id=\"btn-edit\" class=\"btn btn-lg btn-secondary\" name=\"action\" value=\"edit-selected\">" << translate("Edit") << "</button>\n"
      // Label for button to reset the password of the selected user
      "      <button id=\"btn-edit-password\" class=\"btn btn-lg btn-info\" name=\"action\" value=\"password-reset-selected\">" << translate("Password reset") << "</button>\n"
      // Label for button to create a new user
      "      <button id=\"btn-new\" class=\"btn btn-lg btn-warning\" name=\"action\" value=\"new-user\">" << translate("New") << "</button>\n"
      "    </div>\n";
    os <<     "  </form>\n";
  }
  os <<
    "</div>\n";
}

void AdminUserManagementHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  // Page title of the admin user's User Management page
  set_page_title(translate("User Management"));
  set_menu_item(users);
}

void AdminUserManagementHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  const auto action = request.get_post_param("action");
  // std::cout << "Action: \"" << action << "\"\n";
  // for (const auto &m : request.get_post_params())
  //   std::cout << m.first << "->\"" << m.second << "\"\n";
  const auto page = request.get_param("page");
  const bool first_time = page.empty() && request.method != HTTPMethod::post;
  SessionPgDao session_dao;
  const bool is_admin = session_dao.is_admin(get_user_id());
  if (!is_admin) {
    response.content.clear();
    response.content.str("");
    response.status_code = HTTPStatus::unauthorized;
    create_full_html_page_for_standard_response(response);
    return;
  }
  if (action == "new-user") {
    redirect(request, response, get_uri_prefix() + "/edit-user");
    return;
  } else if (!action.empty()) {
    const auto selected = request.extract_array_param_map("user-id");
    if (action == "edit-selected" || action == "password-reset-selected") {
      const auto selected_user = selected.begin();
      if (selected_user != selected.end()) {
        redirect(request,
                 response,
                 get_uri_prefix() + "/edit-user?id=" +
                 std::to_string(selected_user->first));
        return;
      }
    } else if (action == "delete-selected") {
      std::vector<long> user_ids;
      for (const auto &m : selected)
        user_ids.push_back(m.first);
      try {
        session_dao.delete_users(user_ids);
      } catch (const std::exception &e) {
        std::cerr << "Failure deleting users: "
                  << e.what() << '\n';
        delete_users_failed_flag = true;
      }
    }
  }
  try {
    email = request.get_param("search-email");
    nickname = request.get_param("search-nickname");
    const std::string search_type_str = request.get_param("search-type");
    search_type =
      search_type_str == "exact" ? SessionPgDao::exact : SessionPgDao::partial;

    std::map<std::string, std::string> page_param_map;
    page_param_map["search-email"] = UriUtils::uri_encode(email);
    page_param_map["search-nickname"] = UriUtils::uri_encode(nickname);
    page_param_map["search-type"] = UriUtils::uri_encode(search_type_str);

    Pagination pagination(get_uri_prefix() + "/users",
                          page_param_map);

    if (!first_time) {
      const long total_count = session_dao.get_search_users_by_nickname_count(
          email,
          nickname,
          search_type);
      pagination.set_total(total_count);

      if (!page.empty())
        pagination.set_current_page(std::stoul(page));

      auto user_list = session_dao.search_users_by_nickname(
          email,
          nickname,
          search_type,
          pagination.get_offset(),
          pagination.get_limit());
      empty_search_result = user_list.empty();
      build_form(response.content, pagination, user_list);
    } else {
      std::vector<SessionPgDao::user> user_list;
      build_form(response.content, pagination, user_list);
    }
  } catch (const std::logic_error& e) {
    std::cerr << "Error converting string to page number\n";
    throw;
  }
}
