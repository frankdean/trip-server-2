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
#include "track_logging_handler.hpp"
#include "trip_get_options.hpp"
#include "tracking_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/uuid.hpp"
#include <map>
#include <nlohmann/json.hpp>
#include <regex>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::json;

bool TrackLoggingHandler::can_handle(const HTTPServerRequest& request) const
{
  return compare_request_regex(request.uri, "(/rest)?/log_point(\\?.*)?");
}

void TrackLoggingHandler::do_handle_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  (void)response; // unused
  std::string uuid_str;
  std::map<std::string, std::string> params;
  // Must handle both get and post
  if (request.method == HTTPMethod::get) {
    uuid_str = request.get_query_param("uuid");
    params = request.get_query_params();
  } else if (request.method == HTTPMethod::post) {
    const std::string content_type = request.get_header("Content-Type");
    if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
      // std::cout << "Handling as application/x-www-form-urlencoded\n";
      uuid_str = request.get_post_param("uuid");
      params = request.get_post_params();
    } else if (content_type.find("application/json") != std::string::npos &&
               !request.content.empty()) {
      // std::cout << "Request content:\n" << request.content << "\n- - -\n";
      try {
        json j = json::parse(request.content);
        // std::cout << "JSON: \n" << j.dump(4) << '\n';
        for (const auto& el : j.items())
          params[el.key()] = el.value();
        uuid_str = j["uuid"];
      } catch(const std::exception& e) {
        if (GetOptions::verbose_flag)
          std::cerr << "Track log_point Error parsing JSON POST: "
                    << e.what() << '\n';
        std::ostringstream os;
        os << "Track log_point Error parsing JSON POST: "
           << e.what();
        throw BadRequestException(os.str());
      }
    } else {
      if (GetOptions::verbose_flag)
        std::cerr << "Track log_point: empty request" << '\n';
      throw BadRequestException("Track log_point: empty request");
    }
  } else {
    if (GetOptions::verbose_flag)
      std::cerr << "Unexpected HTTP request method: \""
                << request.method_to_str() << "\"\n";
    throw BadRequestException("Unexpected HTTP request method: \""
                              + request.method_to_str() + '"');
  }
  if (!UUID::is_valid(uuid_str)) {
    if (GetOptions::verbose_flag)
      std::cerr << "Track log_point: invalid UUID" << '\n';
    throw BadRequestException("Track log_point: invalid UUID");
  }
  TrackPgDao dao(elevation_service);
  try {
    std::string user_id = dao.get_user_id_by_uuid(uuid_str);
    if (user_id.empty()) {
      if (GetOptions::verbose_flag)
        std::cerr << "UUID not found\n";
      throw BadRequestException("Track log_point: UUID not found");
    }
    TrackPgDao::tracked_location_query_params qp(user_id, params);
    qp.user_id = user_id;
    dao.save_tracked_location(qp);
  } catch (const std::exception &e) {
    if (GetOptions::verbose_flag)
      std::cerr << "Failure searching for user by UUID: "
                << e.what() << '\n';
    throw;
  }
}
