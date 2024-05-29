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
#include "my_account_handler.hpp"
#include "tracking_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <yaml-cpp/yaml.h>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;

void MyAccountHandler::build_form(std::ostream &os)
{
  os <<
    "<div class=\"container-fluid\">\n"
    "<h1>" << get_page_title() << "</h1>\n";
  if (upload_failure) {
    os <<
      "<div id=\"bad-request\" class=\"alert alert-danger\" role=\"alert\">\n"
      // Error message displayed when uploading YAML file containing TripLogger settings fails
       << translate("Upload failed - the most likely reason is that the file is not a valid TripLogger YAML formatted settings file.") << "\n"
      "</div>\n";

  } else if (upload_success) {
    os <<
      "  <div id=\"info-message\" class=\"alert alert-info\" role=\"alert\">\n"
      // Message displayed after successfully uploading YAML file containing TripLogger settings
      "    <p>" << translate("Upload succeeded") << "</p>\n"
      "  </div>\n";
  }
  os <<
    "  <ul>\n"
    "    <li><a href=\""
     << get_uri_prefix() << "/change-password\">"
    // Label for link to change password
     << translate("Change password") << "</a></li>\n"
    "  </ul>\n"
    "  <div class=\"container-fluid bg-light my-3\">\n"
    "    <form  enctype=\"multipart/form-data\" method=\"post\">\n"
    // Describes what the upload settings section of the My Account page can be used for
    "      <h2>" << translate("Upload settings from TripLogger.") << "</h2>\n"
    "      <div>\n"
    // Instructions for uploading a YAML formatted file containing TripLogger settings to be saved
    "        <p>" << translate("Select the TripLogger settings file to be uploaded, then click the upload button.") << "</p>\n"
    "        <input id=\"btn-file-upload\" type=\"file\" accesskey=\"f\" name=\"file\" class=\"btn btn-lg btn-primary mb-2\">\n"
    // Label for button to initiate upload of a YAML file containing settings for the TripLogger app
    "        <button id=\"btn-upload\" type=\"submit\" accesskey=\"u\" name=\"action\" value=\"upload\" class=\"btn btn-lg btn-success mb-2\">" << translate("Upload") << "</button>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n";
}

void MyAccountHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  set_menu_item(account);
  // Title of the My Account page
  set_page_title(translate("My Account"));
}

void MyAccountHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  const std::string action = request.get_post_param("action");

  try {
    if (!action.empty() && action == "upload") {
      // const std::string filename = request.get_post_param("file");
      const auto multiparts = request.multiparts;
      try {
        const auto file_data = multiparts.at("file");
        if (!file_data.body.empty()) {
          // std::cout << file_data.body << '\n';
          YAML::Node node = YAML::Load(file_data.body);
          // std::cout << node << '\n';
          // Validate that it looks like a TripLogger settings file
          if (node["userId"] && node["settingProfiles"]
              && node ["noteSuggestions"]) {
            TrackPgDao::triplogger_configuration settings;
            settings.uuid = node["userId"].as<std::string>();
            std::ostringstream ss;
            ss << node;
            settings.tl_settings = ss.str();
            TrackPgDao dao(elevation_service);
            dao.save(get_user_id(), settings);
            upload_success = true;
          } else {
            upload_failure = true;
          }
        }
      } catch (const std::out_of_range &e) {
        std::cerr << "No file uploaded\n";
      }
    }
    build_form(response.content);
  } catch (const YAML::ParserException &e) {
    std::cerr << "Exception parsing TripLogger settings upload: "
              << e.what() << '\n';
    upload_failure = true;
  } catch (const std::out_of_range &e) {
    std::cerr << "No file uploaded\n";
    upload_failure = true;
  } catch (const std::exception &e) {
    std::cerr << "Exception handling TripLogger settings upload request: "
              << e.what() << '\n';
    throw;
  }
}
