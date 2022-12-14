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
#include "trip_request_handler.hpp"
#include "trip_config.hpp"
#include "session_pg_dao.hpp"
#include "trip_application.hpp"
#include "../trip-server-common/src/file_utils.hpp"
#include "../trip-server-common/src/http_request.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/uri_utils.hpp"
#include <string>
#include <boost/locale.hpp>

using namespace fdsd::trip;
using namespace fdsd::utils;
using namespace fdsd::web;
using namespace boost::locale;

const std::string TripRequestHandler::default_url = "/tracks";
const std::string TripRequestHandler::success_url = "/success";

TripRequestHandler::TripRequestHandler(std::shared_ptr<TripConfig> config) :
  HTTPRequestHandler(config->get_application_prefix_url()),
  config(config)
{
}

void TripRequestHandler::handle_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  try {
    do_handle_request(request, response);
  } catch (const BadRequestException &e) {
    handle_bad_request(request, response);
  } catch (const std::invalid_argument &e) {
    handle_bad_request(request, response);
  } catch (const std::out_of_range &e) {
    handle_bad_request(request, response);
  }
}

const std::string TripLoginRequestHandler::login_url =
  "/login";
const std::string TripLoginRequestHandler::login_redirect_cookie_name =
  "trip-login-redirect";
const std::string TripLoginRequestHandler::session_id_cookie_name =
  "TRIP_SESSION_ID";

std::string TripLoginRequestHandler::get_page_title() const
{
  // Title for the login page
  return translate("Login");
}

bool TripLoginRequestHandler::validate_password(
    const std::string email,
    const std::string password) const
{
  SessionPgDao dao;
  return dao.validate_password(email, password);
}

std::string TripLoginRequestHandler::get_user_id_by_email(
    std::string email) const
{
  SessionPgDao dao;
  return dao.get_user_id_by_email(email);
}

const std::string TripLogoutRequestHandler::logout_url =
  "/logout";

TripAuthenticatedRequestHandler::TripAuthenticatedRequestHandler(std::shared_ptr<TripConfig> config) :
  AuthenticatedRequestHandler(config->get_application_prefix_url()),
  config(config),
  menu_item(menu_items::unknown)
{
}

void TripAuthenticatedRequestHandler::append_head_content(std::ostream& os) const
{
  os <<
    // Bootstrap
    // "    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.0/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-gH2yIJqKdNHPEq0n4Mqa/HGKIhSkIHeL5AyhkYV8i59U5AR6csBvApHHNl/vI1Bx\" crossorigin=\"anonymous\">\n"
    "    <link rel=\"stylesheet\" href=\"" << get_uri_prefix() << "/static/bootstrap-" << BOOTSTRAP_VERSION << "-dist/css/bootstrap.min.css\">\n"
    "    <link rel=\"stylesheet\" href=\"" << get_uri_prefix() << "/static/css/trip.css\">\n";
}

void TripAuthenticatedRequestHandler::append_head_title_section(std::ostream& os) const
{
  if (!get_page_title().empty())
    os << "    <title>TRIP - Trip Recording and Itinerary Planner - "
       << get_page_title()
       << "    </title>\n";
}

void TripAuthenticatedRequestHandler::handle_authenticated_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response)
{
  response.content
    << "<h1>Successfully logged in!</h1>\n"
    "<p>Click <a href=\""
    + TripLogoutRequestHandler::logout_url
    + "\">here</a> to logout</p>";
}

void TripAuthenticatedRequestHandler::append_bootstrap_scripts(std::ostream& os) const
{
  // Bootstrap and Popper
  // "    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.0/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-A3rJD856KowSb7dwlZdYEkO39Gagi7vIsF0jrRAoQmDKKtQBHUuLZ9AsSv4jD4Xa\" crossorigin=\"anonymous\"></script>\n"
  // Popper
  // "    <script src=\"https://cdn.jsdelivr.net/npm/@popperjs/core@2.11.5/dist/umd/popper.min.js\" integrity=\"sha384-Xe+8cL9oJa6tN/veChSP7q+mnSPaj5Bcu9mPX5F5xIGE0DVittaqT5lorf0EI7Vk\" crossorigin=\"anonymous\"></script>"
  // Boostrap
  // "    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.0/dist/js/bootstrap.min.js\" integrity=\"sha384-ODmDIVzN+pFdexxHEHFBQH3/9/vQ9uori45z4JjnFsRydbmQbmL5t1tQ0culUzyK\" crossorigin=\"anonymous\"></script>\n"
  os <<
    "    <script type=\"module\" src=\"" << get_uri_prefix() << "/static/bootstrap-" << BOOTSTRAP_VERSION << "-dist/js/bootstrap.bundle.min.js\"></script>\n";
}

void TripAuthenticatedRequestHandler::append_openlayers_scripts(
    std::ostream& os) const
{
  os <<
    // "    <script src=\"https://cdn.jsdelivr.net/npm/ol@v7.1.0/dist/ol.js\"></script>\n";
    "    <script src=\"" << get_uri_prefix() << "/static/openlayers-" << OPENLAYERS_VERSION << "/ol.js\"></script>\n";
}

void TripAuthenticatedRequestHandler::append_body_start(std::ostream& os) const
{
  SessionPgDao session_dao;
  const bool is_admin = session_dao.is_admin(get_user_id());
  const std::string prefix = get_uri_prefix();
  os <<
    "  <body>\n";
  // "  <noscript>\n"
  // "    You must enable JavaScript to use this application.\n"
  // "  </noscript>\n"
  os <<
    "    <nav class=\"navbar navbar-expand-lg navbar-dark bg-dark mb-3\">\n"
    "      <div class=\"container fluid\">\n"
    "        <a class=\"navbar-brand text-white-50\" href=\"" << get_uri_prefix() << "/tracks\">TRIP</a>\n"
    "        <button type=\"button\" class=\"navbar-toggler\" data-bs-toggle=\"collapse\" data-bs-target=\"#navbarSupportedContent\" aria-controls=\"navbarSupportedContent\">\n"
    "          <span class=\"navbar-toggler-icon\"></span>\n"
    "        </button>\n"
    "        <div id=\"navbarSupportedContent\" class=\"collapse navbar-collapse\">\n"
    "          <ul class=\"navbar-nav me-auto mb-2 mb-lg-0\">\n"
    "            <li class=\"nav-item\"><a class=\"nav-link";

  if (get_menu_item() == tracks)
    os << " active";
  os <<
    // Menu item to select the location tracking page
    "\" href=\"" << prefix << "/tracks\">" << translate("Tracking") << "</a></li>\n"
    "            <li class=\"nav-item\"><a class=\"nav-link";
  if (get_menu_item() == tracker_info)
    os << " active";
  os <<
    "\" href=\"" << prefix << "/tracker-info\">"
    // Menu item to select the tracker information page
     << translate("Tracker Info") << "</a></li>\n"
    "            <li class=\"nav-item\"><a class=\"nav-link";
  if (get_menu_item() == track_sharing)
    os << " active";
  os <<
    "\" href=\"" << prefix << "/sharing\">"
    // Menu item to select the track sharing page
     << translate("Track Sharing") << "</a></li>\n"
    "            <li class=\"nav-item\"><a class=\"nav-link";
  if (get_menu_item() == itineraries)
    os << " active";
  os <<
    "\" href=\"" << prefix << "/itineraries\">"
    // Menu item to select the list of itineraries page
     << translate("Itineraries") << "</a></li>\n"
    "            <li class=\"nav-item opacity-50\"><a class=\"nav-link\">" // href=\"" << prefix << "/location\">"
    // Menu item to select the page potentially showing or recording the user's current location
     << translate("Location") << "</a></li>\n";

  if (is_admin) {
    os <<
      "            <li class=\"nav-item opacity-50\"><a class=\"nav-link\">" // href=\"" << prefix << "/users\">"
      // Menu item for an admin user to administer user accounts, create, delete, reset password
       << translate("Users") << "</a></li>\n"
      "            <li class=\"nav-item opacity-50\"><a class=\"nav-link\">" // href=\"" << prefix << "/status\">"
      // Menu item for an admin user to view the system status report
       << translate("Status") << "</a></li>\n";
  }
  os <<
    "            <li class=\"nav-item\"><a class=\"nav-link opacity-50\">" // href=\"" << prefix << "/account\">"
    // Menu item for a user to administer their own account
     << translate("Account") << "</a></li>\n"
    "            <li class=\"nav-item\"><a class=\"nav-link\" href=\"https://www.fdsd.co.uk/trip-server-2/"
    PACKAGE_NAME "-" PACKAGE_VERSION "/docs/user-guide/\" target=\"_blank\">" <<
    // Menu item linking to the user guide
    translate("Help") << "</a></li>\n"
    "            <li class=\"nav-item\"><a class=\"nav-link\" href=\"" << prefix << "/logout\">" <<
    // Menu item for the user to logout
    translate("Logout") << "</a></li>\n"
    "          </ul>\n"
    "        </div>\n"
    "      </div>\n"
    "    </nav>\n"
    ;
}

void TripAuthenticatedRequestHandler::append_pre_body_end(std::ostream& os) const
{
  append_bootstrap_scripts(os);
}

void TripAuthenticatedRequestHandler::append_footer_content(std::ostream& os) const
{
  std::string package_name = PACKAGE;
  std::transform(package_name.begin(),
                 package_name.end(),
                 package_name.begin(),
                 ::toupper);
  os <<
    "    <div id=\"footer\" class=\"fixed-bottom container-fluid bg-light pt-3 pb-1\">\n"
    "      <footer class=\"footer\">\n"
    "        <div id=\"version\">\n"
    "          <p class=\"text-muted\">" << package_name << ": v<span>" <<  VERSION << "</span></p>\n"
    "        </div>\n"
    "      </footer>\n"
    "    </div>\n";
}

BaseRestHandler::BaseRestHandler(std::shared_ptr<TripConfig> config) :
  TripAuthenticatedRequestHandler(config)
{
}

void BaseRestHandler::set_content_headers(HTTPServerResponse& response) const
{
  response.set_header("Content-Length", std::to_string(response.content.str().length()));
  response.set_header("Content-Type", get_mime_type("json"));
  response.set_header("Cache-Control", "no-cache");
}

BaseMapHandler::BaseMapHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config)
{
}

void BaseMapHandler::append_head_content(std::ostream& os) const
{
  os <<
    // Bootstrap
    // "    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.0/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-gH2yIJqKdNHPEq0n4Mqa/HGKIhSkIHeL5AyhkYV8i59U5AR6csBvApHHNl/vI1Bx\" crossorigin=\"anonymous\">\n"
    "    <link rel=\"stylesheet\" href=\"" << get_uri_prefix() << "/static/bootstrap-" << BOOTSTRAP_VERSION << "-dist/css/bootstrap.min.css\">\n"
    // OpenLayers
    // "    <link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/npm/ol@v7.1.0/ol.css\">\n"
    "    <link rel=\"stylesheet\" href=\"" << get_uri_prefix() << "/static/openlayers-" << OPENLAYERS_VERSION << "/ol.css\">\n"
    "    <link rel=\"stylesheet\" href=\"" << get_uri_prefix() << "/static/css/trip.css\">\n";
}

void BaseMapHandler::append_pre_body_end(std::ostream& os) const
{
  append_bootstrap_scripts(os);
  append_openlayers_scripts(os);
}
