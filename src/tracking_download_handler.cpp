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
#include "tracking_download_handler.hpp"
#include "session_pg_dao.hpp"
#include "tracking_request_handler.hpp"
#include "trip_config.hpp"
#include "../trip-server-common/src/dao_helper.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <nlohmann/json.hpp>
#include <pugixml.hpp>
#include <string>
#include <syslog.h>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::json;
using namespace pugi;

const std::string TrackingDownloadHandler::tracking_download_url = "/download/tracks";

TrackingDownloadHandler::TrackingDownloadHandler(
    std::shared_ptr<TripConfig> config,
    std::shared_ptr<ElevationService> elevation_service) :
  BaseRestHandler(config),
  elevation_service(elevation_service)
{
}

void TrackingDownloadHandler::set_content_headers(HTTPServerResponse& response) const
{
  response.set_header("Content-Length", std::to_string(response.content.str().length()));
  response.set_header("Cache-Control", "no-cache");
  response.set_header("Content-Type", get_mime_type("gpx"));
  std::time_t now = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());
  std::tm tm = *std::localtime(&now);
  std::ostringstream filename;
  filename << "trip-track-"
           << std::put_time(&tm, "%FT%T%z") << ".gpx";
  response.set_header("Content-Disposition", "attachment; filename=\"" +
                      filename.str() + "\"");
}

void TrackingDownloadHandler::handle_download(
    HTTPServerResponse& response,
    const TrackPgDao::tracked_locations_result &locations_result) const
{
  DateTime now;
  xml_document doc;
  doc.load_string(
      "<gpx xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
      "creator=\"TRIP\" version=\"1.1\" "
      "xmlns=\"http://www.topografix.com/GPX/1/1\" "
      "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "
      "http://www.topografix.com/GPX/1/1/gpx.xsd\"></gpx>");
  xml_node decl = doc.prepend_child(node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";
  xml_node gpx = doc.last_child();
  xml_node metadata = gpx.append_child("metadata");
  xml_node time = metadata.append_child("time");
  time.append_child(node_pcdata).set_value(
      now.get_time_as_iso8601_gmt().c_str());
  xml_node trk = gpx.append_child("trk");
  xml_node trkseg = trk.append_child("trkseg");
  for (auto const &loc : locations_result.locations) {
    xml_node trkpt = trkseg.append_child("trkpt");
    trkpt.append_attribute("lat").set_value(loc.latitude);
    trkpt.append_attribute("lon").set_value(loc.longitude);
    if (loc.altitude.has_value())
      trkpt.append_child("ele").text() = loc.altitude.value();
    DateTime tm(loc.time_point);
    trkpt.append_child("time").append_child(node_pcdata).
      set_value(tm.get_time_as_iso8601_gmt().c_str());
    if (loc.hdop.has_value())
      trkpt.append_child("hdop").text() = loc.hdop.value();
  }
  if (config->get_gpx_pretty()) {
    std::string indent;
    int level = config->get_gpx_indent();
    while (level-- > 0)
      indent.append(" ");
    doc.save(response.content, indent.c_str());
  } else {
    doc.save(response.content, "", format_raw);
  }
}

void TrackingDownloadHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  TrackPgDao::location_search_query_params q;
  // q.user_id = get_user_id();
  q = TrackPgDao::location_search_query_params(get_user_id(),
                                               request.get_query_params());
  SessionPgDao session_dao;
  try {
    json j = q;
    // std::cout << "Saving JSON: " << j.dump(4) << '\n';
    session_dao.save_value(get_session_id(),
                           SessionPgDao::tracks_query_key,
                           j.dump());
  } catch (const std::exception& e) {
    std::cerr << "Failed to save query parameters in session: "
              << e.what() << '\n';
    syslog(LOG_ERR, "Failed to save query parameters in session: %s",
           e.what());
  }

  // std::cout << "Query object: " << q << "\n- - -\n";
  // for (auto const& p : q.query_params()) {
  //   std::cout << p.first << " -> " << p.second << '\n';
  // }

  TrackPgDao dao(elevation_service);
  TrackPgDao::tracked_locations_result locations_result;
  q.page = 1;
  q.order = dao_helper::ascending;
  q.page_offset = 0;
  q.page_size = -1;
  locations_result = dao.get_tracked_locations(
      q,
      config->get_maximum_location_tracking_points());

  handle_download(response, locations_result);
}
