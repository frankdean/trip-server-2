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
#include "itinerary_import_handler.hpp"
#include "elevation_tile.hpp"
#include "itinerary_pg_dao.hpp"
#include "../trip-server-common/src/get_options.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <yaml-cpp/yaml.h>
#include <sstream>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;

void ItineraryImportHandler::build_form(web::HTTPServerResponse& response)
{
  response.content <<
    "<div class=\"container-fluid bg-light my-3\">\n"
    // Title of the page for importing a YAML file contain a full itinerary
    "  <h1 class=\"pt-2\">" << translate("Itinerary Import") << "</h1>\n"
    "  <form id=\"form\" enctype=\"multipart/form-data\" method=\"post\">\n"
    "    <div class=\"col-lg-6\">\n"
    // Instructions for uploading a YAML file contain a full itinerary
    "      <p>" << translate("Select the YAML file to be uploaded, then click the upload button.") << "</p>\n"
    "        <input id=\"btn-file-upload\" type=\"file\" accesskey=\"f\" name=\"file\" class=\"btn btn-lg btn-primary\">\n"
    "      </div>\n"

    "      <div class=\"col-12 py-3\" arial-label=\"Form buttons\">\n"
    // Label for button to upload a YAML file containing a full itinerary
    "        <button id=\"btn-upload\" type=\"submit\" accesskey=\"u\" name=\"action\" value=\"upload\" class=\"btn btn-lg btn-success\">" << translate("Upload") << "</button>\n"
    // Label for button to cancel uploading a file containing a full itinerary
    "        <button id=\"btn-cancel\" type=\"submit\" accesskey=\"c\" name=\"action\" value=\"cancel\" class=\"btn btn-lg btn-danger\" formnovalidate>" << translate("Cancel") << "</button>\n"
    "      </div>\n"
    "    </div>\n"
    "  </form>\n"
    "</div>\n";
}

long ItineraryImportHandler::duplicate_itinerary(
    std::string user_id,
    ItineraryPgDao::itinerary_complete& itinerary,
    std::shared_ptr<ElevationService> elevation_service)
{
  if (itinerary.id.has_value()) {
    // Text appended to an itinerary title when it is created as a copy of
    // another itinerary.  The first parameter is the original title with
    // the second parameter the original itinerary ID.
    std::ostringstream os;
    os << format(translate("{1}—Copy of {2}"))
      % itinerary.title
      % itinerary.id.value();
    itinerary.title = os.str();
    itinerary.id = std::optional<long>();
  }

  ItineraryPgDao dao;
  long itinerary_id = dao.save(user_id, itinerary);

  if (!itinerary.shares.empty()) {
    std::vector<ItineraryPgDao::itinerary_share> new_shares;
    TrackPgDao tracking_dao(elevation_service);
    for (const auto &sh : itinerary.shares) {
      try {
        const std::string shared_to_id_str = tracking_dao.get_user_id_by_nickname(sh.nickname);
        ItineraryPgDao::itinerary_share share(sh);
        share.shared_to_id = std::stol(shared_to_id_str);
        new_shares.push_back(share);
      } catch (const std::out_of_range &e) {
        // ignore
        // std::cout << "Failed to find nickname: \"" << sh.nickname << "\"\n";
      }
    }
    if (!new_shares.empty())
      dao.save(user_id, itinerary_id, new_shares);
  }
  dao.create_routes(user_id, itinerary_id, itinerary.routes);
  dao.create_waypoints(user_id, itinerary_id, itinerary.waypoints);
  dao.create_tracks(user_id, itinerary_id, itinerary.tracks);
  return itinerary_id;
}

void ItineraryImportHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  if (request.method == HTTPMethod::get) {
    build_form(response);
    return;
  }
  const std::string action = request.get_post_param("action");
  if (action == "cancel") {
    redirect(request, response, get_uri_prefix() + "/itineraries");
    return;
  }

  const auto multiparts = request.multiparts;
  try {
    const auto file_data = multiparts.at("file");
    // std::cout << "body: " << file_data.body << '\n';
    if (!file_data.body.empty()) {

      YAML::Node node = YAML::Load(file_data.body);
      // std::cout << node << '\n';

      ItineraryPgDao::itinerary_complete
        itinerary(node.as<ItineraryPgDao::itinerary_complete>());

      const auto itinerary_id =
        ItineraryImportHandler::duplicate_itinerary(get_user_id(), itinerary, elevation_service);

      redirect(request, response,
               get_uri_prefix() + "/itinerary?id=" +
               std::to_string(itinerary_id));
      return;
    }
  } catch (const std::out_of_range &e) {
    if (GetOptions::verbose_flag)
      std::cerr << "No file uploaded\n";
  } catch (const std::exception &e) {
    if (GetOptions::verbose_flag)
      std::cerr << "Exception handling file upload: "
                << e.what() << '\n';
    redirect(request,
             response,
             get_uri_prefix() + "/itineraries?error=itinerary-upload-failed");
    return;
  }
  redirect(request, response, get_uri_prefix() + "/itineraries");
}
