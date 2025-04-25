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
#include "download_triplogger_configuration_handler.hpp"
#include "tracking_pg_dao.hpp"
#include "trip_config.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <yaml-cpp/yaml.h>

using namespace fdsd::trip;
using namespace fdsd::web;

void DownloadTripLoggerConfigurationHandler::
    set_content_headers(HTTPServerResponse& response) const
{
  response.set_header("Content-Length",
                      std::to_string(response.content.str().length()));
  response.set_header("Cache-Control", "no-cache");
  response.set_header("Content-Type", get_mime_type("yaml"));
  std::time_t now = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());
  std::tm tm = *std::localtime(&now);
  std::ostringstream filename;
  filename <<
    std::put_time(&tm, "triplogger-settings-%FT%T%z") << ".yaml";
  response.set_header("Content-Disposition", "attachment; filename=\"" +
                      filename.str() + "\"");
}

void DownloadTripLoggerConfigurationHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  (void)request; // unused
  TrackPgDao dao(elevation_service);
  TrackPgDao::triplogger_configuration c =
    dao.get_triplogger_configuration(get_user_id());
  YAML::Node node;
  if (!c.tl_settings.empty()) {
    node = YAML::Load(c.tl_settings);
  } else {
    node = config->create_default_triplogger_configuration();
  }
  node["userId"] = c.uuid;
  // std::cout << node << '\n';
  response.content << node;
}
