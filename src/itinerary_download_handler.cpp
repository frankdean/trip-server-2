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
#include "itinerary_download_handler.hpp"
#include "trip_config.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <sstream>
#include <pugixml.hpp>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::json;
using namespace pugi;

void ItineraryDownloadHandler::handle_gpx_download(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  DateTime now;
  std::map<long, std::string> route_map =
    request.extract_array_param_map("route");
  std::map<long, std::string> waypoint_map =
    request.extract_array_param_map("waypoint");
  std::map<long, std::string> track_map =
    request.extract_array_param_map("track");
  std::vector<long> route_ids;
  for (const auto &m : route_map) {
    if (m.second == "on")
      route_ids.push_back(m.first);
  }
  std::vector<long> waypoint_ids;
  for (const auto &m : waypoint_map) {
    if (m.second == "on")
      waypoint_ids.push_back(m.first);
  }
  std::vector<long> track_ids;
  for (const auto &m : track_map) {
    if (m.second == "on")
      track_ids.push_back(m.first);
  }
  ItineraryPgDao dao;
  const auto routes = dao.get_routes(get_user_id(), itinerary_id, route_ids);
  const auto waypoints = dao.get_waypoints(get_user_id(), itinerary_id, waypoint_ids);
  const auto tracks = dao.get_tracks(get_user_id(), itinerary_id, track_ids);
  // std::cout << "Finished reading from database\n";
  // std::cout << "Tracks:\n";
  // for (const auto &t : tracks) {
  //   std::cout << "Track ID: " << t->id << '\n';
  //   for (const auto &ts : t->segments) {
  //     std::cout << "Track segment ID: " << ts->id << '\n';
  //     for (const auto &p : ts->points) {
  //       std::cout << "Point: " << p->id << '\n';
  //     }
  //   }
  // }

  xml_document doc;
  doc.load_string(
      "<gpx xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
      "creator=\"TRIP\" version=\"1.1\" "
      "xmlns=\"http://www.topografix.com/GPX/1/1\" "
      "xmlns:gpxx=\"http://www.garmin.com/xmlschemas/GpxExtensions/v3\" "
      "xmlns:wptx1=\"http://www.garmin.com/xmlschemas/WaypointExtension/v1\" "
      "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "
      "http://www.topografix.com/GPX/1/1/gpx.xsd "
      "http://www.garmin.com/xmlschemas/GpxExtensions/v3 "
      "http://www8.garmin.com/xmlschemas/GpxExtensionsv3.xsd "
      "http://www.garmin.com/xmlschemas/WaypointExtension/v1 "
      "http://www8.garmin.com/xmlschemas/WaypointExtensionv1.xsd\"></gpx>");
  xml_node decl = doc.prepend_child(node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";
  xml_node gpx = doc.last_child();
  xml_node metadata = gpx.append_child("metadata");
  xml_node time = metadata.append_child("time");
  time.append_child(node_pcdata).set_value(
      now.get_time_as_iso8601_gmt().c_str());
  // Waypoints
  for (const auto &wpt : waypoints) {
    xml_node wpt_node = gpx.append_child("wpt");
    wpt_node.append_attribute("lon").set_value(std::to_string(wpt->longitude).c_str());
    wpt_node.append_attribute("lat").set_value(std::to_string(wpt->latitude).c_str());
    if (wpt->altitude.first)
      wpt_node.append_child("ele").append_child(node_pcdata)
        .set_value(std::to_string(wpt->altitude.second).c_str());
    if (wpt->time.first) {
      DateTime time(wpt->time.second);
      wpt_node.append_child("time").append_child(node_pcdata)
        .set_value(time.get_time_as_iso8601_gmt().c_str());
    }
    if (wpt->name.first) {
      wpt_node.append_child("name").append_child(node_pcdata)
        .set_value(wpt->name.second.c_str());
    } else {
      wpt_node.append_child("name").append_child(node_pcdata)
        .set_value(("WPT: " + std::to_string(wpt->id.second)).c_str());
    }
    if (wpt->comment.first)
      wpt_node.append_child("cmt").append_child(node_pcdata)
        .set_value(wpt->comment.second.c_str());
    if (wpt->description.first)
      wpt_node.append_child("desc").append_child(node_pcdata)
        .set_value(wpt->description.second.c_str());
    if (wpt->symbol.first)
      wpt_node.append_child("sym").append_child(node_pcdata)
        .set_value(wpt->symbol.second.c_str());
    if (wpt->type.first)
      wpt_node.append_child("type").append_child(node_pcdata)
        .set_value(wpt->type.second.c_str());
    const auto trip_config = std::static_pointer_cast<TripConfig>(config);
    // The 'color' element used by OsmAnd is not valid in the XSDs, so optional config
    const bool allow_invalid_xsd = trip_config->get_allow_invalid_xsd();
    if ((allow_invalid_xsd && wpt->color.first) || wpt->avg_samples.first) {
      xml_node ext_node = wpt_node.append_child("extensions");
      if (allow_invalid_xsd && wpt->color.first)
        ext_node.append_child("color").append_child(node_pcdata)
          .set_value(wpt->color.second.c_str());
      if (wpt->avg_samples.first)
        ext_node.append_child("wptx1:WaypointExtension")
          .append_child("wptx1:Samples").append_child(node_pcdata)
          .set_value(std::to_string(wpt->avg_samples.second).c_str());
    }
  }
  // Routes
  for (const auto &rte : routes) {
    xml_node rte_node = gpx.append_child("rte");
    if (rte->name.first) {
      rte_node.append_child("name").append_child(node_pcdata)
        .set_value(rte->name.second.c_str());
    } else {
      rte_node.append_child("name").append_child(node_pcdata)
        .set_value(("RTE: " + std::to_string(rte->id.second)).c_str());
    }
    if (rte->color.first) {
      xml_node rt_ext_node = rte_node.append_child("extensions")
        .append_child("gpxx:RouteExtension");
      // IsAutoNamed is mandatory for a RouteExtension in XSD
      rt_ext_node.append_child("gpxx:IsAutoNamed").append_child(node_pcdata)
        .set_value("false");
      rt_ext_node.append_child("gpxx:DisplayColor").append_child(node_pcdata)
        .set_value(rte->color.second.c_str());
    }
    int rtept_count = 0;
    for (const auto &p : rte->points) {
      xml_node rtept_node = rte_node.append_child("rtept");
      rtept_node.append_attribute("lon").set_value(std::to_string(p->longitude).c_str());
      rtept_node.append_attribute("lat").set_value(std::to_string(p->latitude).c_str());
      if (p->altitude.first)
        rtept_node.append_child("ele").append_child(node_pcdata)
          .set_value(std::to_string(p->altitude.second).c_str());
      if (p->name.first) {
        rtept_node.append_child("name").append_child(node_pcdata)
          .set_value(p->name.second.c_str());
      } else {
        // format unnamed points as 001 etc.
        std::ostringstream os;
        os << std::setfill('0') << std::setw(3) << ++rtept_count;
        rtept_node.append_child("name").append_child(node_pcdata)
          .set_value(os.str().c_str());
      }
      if (p->comment.first)
        rtept_node.append_child("cmt").append_child(node_pcdata)
          .set_value(p->comment.second.c_str());
      if (p->description.first)
        rtept_node.append_child("desc").append_child(node_pcdata)
          .set_value(p->description.second.c_str());
      if (p->symbol.first)
        rtept_node.append_child("sym").append_child(node_pcdata)
          .set_value(p->symbol.second.c_str());
    }
  }
  // Tracks
  for (const auto &trk : tracks) {
    xml_node trk_node = gpx.append_child("trk");
    if (trk->name.first) {
      trk_node.append_child("name").append_child(node_pcdata)
        .set_value(trk->name.second.c_str());
    } else {
      trk_node.append_child("name").append_child(node_pcdata)
        .set_value(("TRK: " + std::to_string(trk->id.second)).c_str());
    }
    if (trk->color.first) {
      xml_node ext_node = trk_node.append_child("extensions")
        .append_child("gpxx:TrackExtension");
      ext_node.append_child("gpxx:DisplayColor").append_child(node_pcdata)
        .set_value(trk->color.second.c_str());
    }
    for (const auto &trkseg : trk->segments) {
      xml_node trkseg_node = trk_node.append_child("trkseg");
      for (auto const &p : trkseg->points) {
        xml_node trkpt_node = trkseg_node.append_child("trkpt");
        trkpt_node.append_attribute("lon").set_value(std::to_string(p->longitude).c_str());
        trkpt_node.append_attribute("lat").set_value(std::to_string(p->latitude).c_str());
        if (p->altitude.first) {
          trkpt_node.append_child("ele").append_child(node_pcdata).set_value(
              std::to_string(p->altitude.second).c_str());
        }
        if (p->time.first) {
          DateTime time(p->time.second);
          trkpt_node.append_child("time").append_child(node_pcdata).
            set_value(time.get_time_as_iso8601_gmt().c_str());
        }
        if (p->hdop.first) {
          trkpt_node.append_child("hdop").
            append_child(node_pcdata).set_value(
                std::to_string(p->hdop.second).c_str());
        }
      }
    }
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

void ItineraryDownloadHandler::set_content_headers(HTTPServerResponse& response) const
{
  response.set_header("Content-Length", std::to_string(response.content.str().length()));
  response.set_header("Cache-Control", "no-cache");
  response.set_header("Content-Type", get_mime_type("gpx"));
  std::time_t now = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());
  std::tm tm = *std::localtime(&now);
  std::ostringstream filename;
  filename << "trip-itinerary-" << itinerary_id << ".gpx";
    // std::put_time(&tm, "%FT%T%z") << ".gpx";
  response.set_header("Content-Disposition", "attachment; filename=\"" +
                      filename.str() + "\"");
}

void ItineraryDownloadHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  // auto pp = request.get_post_params();
  // for (auto const &p : pp) {
  //   std::cout << "param: \"" << p.first << "\" -> \"" << p.second << "\"\n";
  // }
  try {
    itinerary_id = std::stol(request.get_param("id"));
    handle_gpx_download(request, response);
  } catch (const std::exception &e) {
    std::cerr << "Exception downloading itinerary: "
              << e.what() << '\n';
  }
}
