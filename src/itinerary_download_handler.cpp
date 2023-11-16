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
#include "itinerary_handler.hpp"
#include "trip_config.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <algorithm>
#include <sstream>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::json;
using namespace pugi;

void ItineraryDownloadHandler::handle_gpx_download(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  const DateTime now;
  auto features = ItineraryHandler::get_selected_feature_ids(request);
  ItineraryPgDao dao;
  const auto routes = dao.get_routes(get_user_id(),
                                     itinerary_id, features.routes);
  const auto waypoints = dao.get_waypoints(get_user_id(),
                                           itinerary_id, features.waypoints);
  const auto tracks = dao.get_tracks(get_user_id(),
                                     itinerary_id, features.tracks);
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

  const auto trip_config = std::static_pointer_cast<TripConfig>(config);
  // allow_invalid_xsd now effectively means include OSMAnd attributes
  const bool allow_invalid_xsd = trip_config->get_allow_invalid_xsd();
  std::stringstream gpx_attributes;
  gpx_attributes <<
    "<gpx xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
    "creator=\"TRIP\" version=\"1.1\" "
    "xmlns=\"http://www.topografix.com/GPX/1/1\" ";
  if (allow_invalid_xsd)
    gpx_attributes << "xmlns:osmand=\"https://osmand.net\" ";
  gpx_attributes <<
    "xmlns:gpxx=\"http://www.garmin.com/xmlschemas/GpxExtensions/v3\" "
    "xmlns:wptx1=\"http://www.garmin.com/xmlschemas/WaypointExtension/v1\" "
    "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "
    "http://www.topografix.com/GPX/1/1/gpx.xsd "
    "http://www.garmin.com/xmlschemas/GpxExtensions/v3 "
    "http://www8.garmin.com/xmlschemas/GpxExtensionsv3.xsd "
    "http://www.garmin.com/xmlschemas/WaypointExtension/v1 "
    "http://www8.garmin.com/xmlschemas/WaypointExtensionv1.xsd\"></gpx>";
  xml_document doc;
  doc.load(gpx_attributes);
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
    wpt_node.append_attribute("lon").set_value(wpt.longitude);
    wpt_node.append_attribute("lat").set_value(wpt.latitude);
    if (wpt.altitude.first) {
      wpt_node.append_child("ele").text() = wpt.altitude.second;
    }
    if (wpt.time.first) {
      DateTime time(wpt.time.second);
      wpt_node.append_child("time").append_child(node_pcdata)
        .set_value(time.get_time_as_iso8601_gmt().c_str());
    }
    if (wpt.name.first) {
      wpt_node.append_child("name").append_child(node_pcdata)
        .set_value(wpt.name.second.c_str());
    } else {
      wpt_node.append_child("name").append_child(node_pcdata)
        .set_value(("WPT: " + std::to_string(wpt.id.second)).c_str());
    }
    if (wpt.comment.first)
      wpt_node.append_child("cmt").append_child(node_pcdata)
        .set_value(wpt.comment.second.c_str());
    if (wpt.description.first)
      wpt_node.append_child("desc").append_child(node_pcdata)
        .set_value(wpt.description.second.c_str());
    if (wpt.symbol.first)
      wpt_node.append_child("sym").append_child(node_pcdata)
        .set_value(wpt.symbol.second.c_str());
    if (wpt.type.first)
      wpt_node.append_child("type").append_child(node_pcdata)
        .set_value(wpt.type.second.c_str());
    if ((allow_invalid_xsd && wpt.color.first) || wpt.avg_samples.first) {
      xml_node ext_node = wpt_node.append_child("extensions");
      if (allow_invalid_xsd && wpt.color.first)
        ext_node.append_child("osmand:color").append_child(node_pcdata)
          .set_value(wpt.color.second.c_str());
      if (wpt.avg_samples.first)
        ext_node.append_child("wptx1:WaypointExtension")
          .append_child("wptx1:Samples").text() = wpt.avg_samples.second;
    }
  }
  // Routes
  for (const auto &rte : routes) {
    xml_node rte_node = gpx.append_child("rte");
    if (rte.name.first) {
      rte_node.append_child("name").append_child(node_pcdata)
        .set_value(rte.name.second.c_str());
    } else {
      rte_node.append_child("name").append_child(node_pcdata)
        .set_value(("RTE: " + std::to_string(rte.id.second)).c_str());
    }
    if (rte.color_key.first) {
      xml_node rt_ext_node = rte_node.append_child("extensions")
        .append_child("gpxx:RouteExtension");
      // IsAutoNamed is mandatory for a RouteExtension in XSD
      rt_ext_node.append_child("gpxx:IsAutoNamed").append_child(node_pcdata)
        .set_value("false");
      rt_ext_node.append_child("gpxx:DisplayColor").append_child(node_pcdata)
        .set_value(rte.color_key.second.c_str());
    }
    int rtept_count = 0;
    for (const auto &p : rte.points) {
      xml_node rtept_node = rte_node.append_child("rtept");
      rtept_node.append_attribute("lon").set_value(p.longitude);
      rtept_node.append_attribute("lat").set_value(p.latitude);
      if (p.altitude.first)
        rtept_node.append_child("ele").text() = p.altitude.second;
      if (p.name.first) {
        rtept_node.append_child("name").append_child(node_pcdata)
          .set_value(p.name.second.c_str());
      } else {
        // format unnamed points as 001 etc.
        std::ostringstream os;
        os << std::setfill('0') << std::setw(3) << ++rtept_count;
        rtept_node.append_child("name").append_child(node_pcdata)
          .set_value(os.str().c_str());
      }
      if (p.comment.first)
        rtept_node.append_child("cmt").append_child(node_pcdata)
          .set_value(p.comment.second.c_str());
      if (p.description.first)
        rtept_node.append_child("desc").append_child(node_pcdata)
          .set_value(p.description.second.c_str());
      if (p.symbol.first)
        rtept_node.append_child("sym").append_child(node_pcdata)
          .set_value(p.symbol.second.c_str());
    }
  }
  // Tracks
  for (const auto &trk : tracks) {
    xml_node trk_node = gpx.append_child("trk");
    if (trk.name.first) {
      trk_node.append_child("name").append_child(node_pcdata)
        .set_value(trk.name.second.c_str());
    } else {
      trk_node.append_child("name").append_child(node_pcdata)
        .set_value(("TRK: " + std::to_string(trk.id.second)).c_str());
    }
    if (trk.color_key.first) {
      xml_node ext_node = trk_node.append_child("extensions")
        .append_child("gpxx:TrackExtension");
      ext_node.append_child("gpxx:DisplayColor").append_child(node_pcdata)
        .set_value(trk.color_key.second.c_str());
    }
    for (const auto &trkseg : trk.segments) {
      xml_node trkseg_node = trk_node.append_child("trkseg");
      for (auto const &p : trkseg.points) {
        xml_node trkpt_node = trkseg_node.append_child("trkpt");
        trkpt_node.append_attribute("lon").set_value(p.longitude);
        trkpt_node.append_attribute("lat").set_value(p.latitude);
        if (p.altitude.first) {
          trkpt_node.append_child("ele").text() = p.altitude.second;
        }
        if (p.time.first) {
          DateTime time(p.time.second);
          trkpt_node.append_child("time").append_child(node_pcdata).
            set_value(time.get_time_as_iso8601_gmt().c_str());
        }
        if (p.hdop.first) {
          trkpt_node.append_child("hdop").text() = p.hdop.second;
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

void ItineraryDownloadHandler::append_xml(
    xml_node &target, const std::string &xml)
{
  xml_document tmp;
  tmp.load_string(xml.c_str());
  for (auto child = tmp.first_child(); child; child = child.next_sibling())
    target.append_copy(child);
}

void ItineraryDownloadHandler::append_kml_route_styles(xml_node &target) {
  append_xml(
      target,
      "    <!-- Normal route style -->\n"
      "    <Style id=\"route_n\">\n"
      "      <IconStyle>\n"
      "        <Icon>\n"
      "          <href>http://earth.google.com/images/kml-icons/track-directional/track-none.png</href>\n"
      "        </Icon>\n"
      "      </IconStyle>\n"
      "    </Style>\n"
      "    <!-- Highlighted route style -->\n"
      "    <Style id=\"route_h\">\n"
      "      <IconStyle>\n"
      "        <scale>1.2</scale>\n"
      "        <Icon>\n"
      "          <href>http://earth.google.com/images/kml-icons/track-directional/track-none.png</href>\n"
      "        </Icon>\n"
      "      </IconStyle>\n"
      "    </Style>\n"
      "    <StyleMap id=\"route\">\n"
      "      <Pair>\n"
      "        <key>normal</key>\n"
      "        <styleUrl>#route_n</styleUrl>\n"
      "      </Pair>\n"
      "      <Pair>\n"
      "        <key>highlight</key>\n"
      "        <styleUrl>#route_h</styleUrl>\n"
      "      </Pair>\n"
      "    </StyleMap>\n");
}

void ItineraryDownloadHandler::append_kml_track_styles(xml_node &target) {
  append_xml(
      target,
      "    <!-- Normal track style -->\n"
      "    <Style id=\"track_n\">\n"
      "      <IconStyle>\n"
      "        <scale>.5</scale>\n"
      "        <Icon>\n"
      "          <href>http://earth.google.com/images/kml-icons/track-directional/track-none.png</href>\n"
      "        </Icon>\n"
      "      </IconStyle>\n"
      "      <LabelStyle>\n"
      "        <scale>0</scale>\n"
      "      </LabelStyle>\n"
      "    </Style>\n"
      "    <!-- Highlighted track style -->\n"
      "    <Style id=\"track_h\">\n"
      "      <IconStyle>\n"
      "        <scale>1.2</scale>\n"
      "        <Icon>\n"
      "          <href>http://earth.google.com/images/kml-icons/track-directional/track-none.png</href>\n"
      "        </Icon>\n"
      "      </IconStyle>\n"
      "    </Style>\n"
      "    <StyleMap id=\"track\">\n"
      "      <Pair>\n"
      "        <key>normal</key>\n"
      "        <styleUrl>#track_n</styleUrl>\n"
      "      </Pair>\n"
      "      <Pair>\n"
      "        <key>highlight</key>\n"
      "        <styleUrl>#track_h</styleUrl>\n"
      "      </Pair>\n"
      "    </StyleMap>\n"
    );
}

void ItineraryDownloadHandler::append_kml_styles(xml_node &target)
{
  append_xml(
      target,
      "    <!-- Normal waypoint style -->\n"
      "    <Style id=\"waypoint_n\">\n"
      "      <IconStyle>\n"
      "        <Icon>\n"
      "          <href>http://maps.google.com/mapfiles/kml/pal4/icon61.png</href>\n"
      "        </Icon>\n"
      "      </IconStyle>\n"
      "    </Style>\n"
      "    <!-- Highlighted waypoint style -->\n"
      "    <Style id=\"waypoint_h\">\n"
      "      <IconStyle>\n"
      "        <scale>1.2</scale>\n"
      "        <Icon>\n"
      "          <href>http://maps.google.com/mapfiles/kml/pal4/icon61.png</href>\n"
      "        </Icon>\n"
      "      </IconStyle>\n"
      "    </Style>\n"
      "    <StyleMap id=\"waypoint\">\n"
      "      <Pair>\n"
      "        <key>normal</key>\n"
      "        <styleUrl>#waypoint_n</styleUrl>\n"
      "      </Pair>\n"
      "      <Pair>\n"
      "        <key>highlight</key>\n"
      "        <styleUrl>#waypoint_h</styleUrl>\n"
      "      </Pair>\n"
      "    </StyleMap>\n");
}

void ItineraryDownloadHandler::append_kml_waypoints(
    xml_node &kml, const std::vector<ItineraryPgDao::waypoint> &waypoints)
{
  if (waypoints.empty()) return;
  auto folder = kml.append_child("Folder");
  folder.append_child("name").append_child(node_pcdata)
    .set_value("Waypoints");
  for (const auto &w : waypoints) {
    auto placemark = folder.append_child("Placemark");
    if (w.name.first || w.id.first) {
      auto name = placemark.append_child("name").append_child(node_pcdata);
      if (w.name.first)
        name.set_value(w.name.second.c_str());
      else if (w.id.first)
        name.set_value(("WPT: " + std::to_string(w.id.second)).c_str());
    }
    if (w.comment.first && !w.comment.second.empty()) {
      placemark.append_child("description").append_child(node_pcdata)
        .set_value(w.comment.second.c_str());
    }
    if (w.time.first) {
      DateTime dt(w.time.second);
      placemark.append_child("TimeStamp").append_child("when")
        .append_child(node_pcdata)
        .set_value(dt.get_time_as_iso8601_gmt().c_str());
    }
    placemark.append_child("styleUrl").append_child(node_pcdata)
      .set_value("#waypoint");
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6)
       << w.longitude << ',' << w.latitude;
    if (w.altitude.first)
      ss << ',' << std::setprecision(2) << w.altitude.second;
    placemark.append_child("Point").append_child("coordinates")
      .append_child(node_pcdata)
      .set_value(ss.str().c_str());
  }
}

void ItineraryDownloadHandler::append_kml_routes(
    xml_node &kml, std::vector<ItineraryPgDao::route> &routes)
{
  const int tilt = 66;
  if (routes.empty()) return;
  auto folder = kml.append_child("Folder");
  folder.append_child("name").append_child(node_pcdata)
    .set_value("Routes");
  for (auto &r : routes) {
    r.calculate_statistics();
    auto route_folder = folder.append_child("Folder");
    if (r.name.first || r.id.first) {
      auto name = route_folder.append_child("name").append_child(node_pcdata);
      if (r.name.first)
        name.set_value(r.name.second.c_str());
      else if (r.id.first)
        name.set_value(("RTE: " + std::to_string(r.id.second)).c_str());
    }
    auto points_folder = route_folder.append_child("Folder");
    points_folder.append_child("name").append_child(node_pcdata)
      .set_value("Points");
    int count = 0;
    for (const auto &p : r.points) {
      auto placemark = points_folder.append_child("Placemark");
      std::stringstream ss;
      ss << std::setw(3) << std::setfill('0') << ++count;
      placemark.append_child("name").append_child(node_pcdata)
        .set_value(ss.str().c_str());
      placemark.append_child("snippet");
      std::stringstream data;
      data
        << "\n<table>\n"
        "<tr><td>Longitude: " << std::fixed << std::setprecision(6)
        << p.longitude << " </td></tr>\n"
        "<tr><td>Latitude: " <<  p.latitude << " </td></tr>\n";
      if (p.altitude.first) {
        data << "<tr><td>Altitude: " << std::setprecision(3) << p.altitude.second
             << " meters </td></tr>\n";
      }
      data << "</table>\n";
      placemark.append_child("description").append_child(node_cdata)
        .set_value(data.str().c_str());
      auto look_at = placemark.append_child("LookAt");
      look_at.append_child("longitude").append_child(node_pcdata)
        .set_value(std::to_string(p.longitude).c_str());
      look_at.append_child("latitude").append_child(node_pcdata)
        .set_value(std::to_string(p.latitude).c_str());
      look_at.append_child("tilt").append_child(node_pcdata)
        .set_value(std::to_string(tilt).c_str());
      placemark.append_child("styleUrl").append_child(node_pcdata)
        .set_value("#route");
      std::stringstream loc;
      loc << std::fixed << std::setprecision(6)
          << p.longitude << ','
          << p.latitude;
    if (p.altitude.first)
      loc << ',' << std::setprecision(2) << p.altitude.second;
    placemark.append_child("Point").append_child("coordinates")
      .append_child(node_pcdata)
      .set_value(loc.str().c_str());
    } // for points

    // linestring/path section
    auto path_placemark = route_folder.append_child("Placemark");
    path_placemark.append_child("name").append_child(node_pcdata)
      .set_value("Path");
    path_placemark.append_child("styleUrl").append_child(node_pcdata)
      .set_value("#lineStyle");
    path_placemark.append_child("Style").append_child("LineStyle")
      .append_child("color").append_child(node_pcdata)
      .set_value("ff0000ff");
    auto line_string = path_placemark.append_child("LineString");
    line_string.append_child("tessellate").append_child(node_pcdata)
      .set_value("1");
    std::stringstream cc;
    cc << '\n';
    for (const auto &p : r.points) {
      cc << std::fixed << std::setprecision(6)
         << p.longitude << ','
         << p.latitude;
      if (p.altitude.first)
        cc << ',' << std::setprecision(2) << p.altitude.second;
      cc << '\n';
    }
    line_string.append_child("coordinates").append_child(node_pcdata)
      .set_value(cc.str().c_str());
  } // for routes
}

void ItineraryDownloadHandler::append_kml_tracks(
    xml_node &kml, std::vector<ItineraryPgDao::track> &tracks)
{
  const int tilt = 66;
  if (tracks.empty()) return;
  auto folder = kml.append_child("Folder");
  folder.append_child("name").append_child(node_pcdata)
    .set_value("Tracks");
  for (auto &t : tracks) {
    t.calculate_statistics();
    t.calculate_speed_and_bearing_values();
    auto maximum_speed = t.calculate_maximum_speed();
    auto track_folder = folder.append_child("Folder");
    std::stringstream track_name;
    if (t.name.first) {
      track_name << t.name.second;
    } else {
      track_name << "TRK";
      if (t.id.first)
        track_name << ": " << t.id.second;
    }
    auto name = track_folder.append_child("name").append_child(node_pcdata);
    name.set_value(track_name.str().c_str());
    track_folder.append_child("snippet");
    time_span_type time_span;
    DateTime begin;
    DateTime end;
    for (const auto &ts : t.segments)
      for (const auto &p : ts.points)
        if (p.time.first)
          time_span.update(p.time.second);
    std::pair<bool, double> average_speed;
    if (time_span.is_valid) {
      begin = DateTime(time_span.start);
      end = DateTime(time_span.finish);
      if (t.distance.first) {
        std::chrono::duration<double, std::milli> diff =
          end.time_tp() - begin.time_tp();
        const double hours =
          std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() / 3600000.0;
        average_speed = std::make_pair(true, t.distance.second / hours);
      }
    }
    std::stringstream dd;
    dd <<
      "<table>\n";
    if (t.distance.first) {
      dd << "<tr><td><b>Distance</b> " << std::fixed << std::setprecision(1);
      if (t.distance.second >= 1)
        dd << t.distance.second << " km";
      else
        dd << (t.distance.second * 1000.0) << " meters";
      dd << " </td></tr>\n";
    }
    if (t.lowest.first)
      dd << "<tr><td><b>Min Alt</b> " << std::fixed << std::setprecision(3)
         << t.lowest.second << " meters </td></tr>\n";
    if (t.highest.first)
      dd << "<tr><td><b>Max Alt</b> "<< std::fixed << std::setprecision(3)
         << t.highest.second << " meters </td></tr>\n";
    if (maximum_speed.first) {
      dd << "<tr><td><b>Max Speed</b> " << std::fixed << std::setprecision(1);
      if (maximum_speed.second >= 1)
        dd << maximum_speed.second << " km/hour";
      else
        dd << (maximum_speed.second * 1000.0) << " meters/hour";
      dd << " </td></tr>\n";
    }
    if (average_speed.first) {
      dd << "<tr><td><b>Avg Speed</b> " << std::fixed << std::setprecision(1);
      if (average_speed.second >= 1)
        dd << average_speed.second << " km/hour";
      else
        dd << (average_speed.second * 1000.0) << " meters/hour";
      dd << " </td></tr>\n";
    }
    if (time_span.is_valid) {
      dd <<
        "<tr><td><b>Start Time</b> "
         << begin.get_time_as_iso8601_gmt().c_str()
         << "</td></tr>\n"
        "<tr><td><b>End Time</b> "
         << end.get_time_as_iso8601_gmt().c_str()
         << "</td></tr>\n";
    }
    dd << "</table>";
    track_folder.append_child("description").append_child(node_cdata)
      .set_value(dd.str().c_str());

    if (time_span.is_valid) {
      auto time_span = track_folder.append_child("TimeSpan");
      time_span.append_child("begin").append_child(node_pcdata)
        .set_value(begin.get_time_as_iso8601_gmt().c_str());
      time_span.append_child("end").append_child(node_pcdata)
        .set_value(end.get_time_as_iso8601_gmt().c_str());
    }
    auto points_folder = track_folder.append_child("Folder");
    points_folder.append_child("name").append_child(node_pcdata)
      .set_value("Points");
    int count = 0;
    for (const auto &ts : t.segments) {
      for (const auto &p : ts.points) {
        auto placemark = points_folder.append_child("Placemark");
        std::stringstream ss;
        ss << track_name.str() << '-' << count++;
        placemark.append_child("name").append_child(node_pcdata)
          .set_value(ss.str().c_str());
        placemark.append_child("snippet");
        std::stringstream data;
        data
          << "\n<table>\n"
          "<tr><td>Longitude: " << std::fixed << std::setprecision(6)
          << p.longitude << " </td></tr>\n"
          "<tr><td>Latitude: " <<  p.latitude << " </td></tr>\n";
        if (p.altitude.first)
          data << "<tr><td>Altitude: " << std::setprecision(3) << p.altitude.second
               << " meters </td></tr>\n";
        if (p.speed.first) {
          data << "<tr><td>Speed: " << std::setprecision(1);
          if (p.speed.second >= 1)
            data << p.speed.second << " km/hour";
          else
            data << p.speed.second * 1000.0 << " meters/hour";
          data << " </td></tr>\n";
        }
        if (p.bearing.first)
          data << "<tr><td>Heading: " << std::setprecision(1) << p.bearing.second
               << " </td></tr>\n";
        if (p.time.first) {
          DateTime speed_date(p.time.second);
          data << "<tr><td>Time: " << speed_date.get_time_as_iso8601_gmt()
             << " </td></tr>\n";
        }
        data << "</table>\n";
        placemark.append_child("description").append_child(node_cdata)
          .set_value(data.str().c_str());
        auto look_at = placemark.append_child("LookAt");
        look_at.append_child("longitude").append_child(node_pcdata)
          .set_value(std::to_string(p.longitude).c_str());
        look_at.append_child("latitude").append_child(node_pcdata)
          .set_value(std::to_string(p.latitude).c_str());
        look_at.append_child("tilt").append_child(node_pcdata)
          .set_value(std::to_string(tilt).c_str());

        if (p.time.first) {
          DateTime dt(p.time.second);
          placemark.append_child("TimeStamp").append_child("when")
            .append_child(node_pcdata)
            .set_value(dt.get_time_as_iso8601_gmt().c_str());
        }

        placemark.append_child("styleUrl").append_child(node_pcdata)
          .set_value("#track");
        std::stringstream loc;
        loc << std::fixed << std::setprecision(6)
            << p.longitude << ','
            << p.latitude;
        if (p.altitude.first)
          loc << ',' << std::setprecision(2) << p.altitude.second;
        placemark.append_child("Point").append_child("coordinates")
          .append_child(node_pcdata)
          .set_value(loc.str().c_str());
      } // for points
    } // for segments

    // linestring/path section
    auto path_placemark = track_folder.append_child("Placemark");
    path_placemark.append_child("name").append_child(node_pcdata)
      .set_value("Path");
    path_placemark.append_child("styleUrl").append_child(node_pcdata)
      .set_value("#lineStyle");
    path_placemark.append_child("Style").append_child("LineStyle")
      .append_child("color").append_child(node_pcdata)
      .set_value("ff0000ff");
    auto multi = path_placemark.append_child("MultiGeometry");

    for (const auto &ts : t.segments) {
      auto line_string = multi.append_child("LineString");
      line_string.append_child("tessellate").append_child(node_pcdata)
        .set_value("1");
      std::stringstream cc;
      cc << '\n';
      for (const auto &p : ts.points) {
        cc << std::fixed << std::setprecision(6)
           << p.longitude << ','
           << p.latitude;
        if (p.altitude.first)
          cc << ',' << std::setprecision(2) << p.altitude.second;
        cc << '\n';
      }
      line_string.append_child("coordinates").append_child(node_pcdata)
        .set_value(cc.str().c_str());
    } // for segments (linestring/path section)

  } // for tracks
}

void ItineraryDownloadHandler::handle_kml_download(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  const DateTime now;
  auto features = ItineraryHandler::get_selected_feature_ids(request);
  ItineraryPgDao dao;
  auto routes = dao.get_routes(get_user_id(),
                                     itinerary_id, features.routes);
  const auto waypoints = dao.get_waypoints(get_user_id(),
                                           itinerary_id, features.waypoints);
  auto tracks = dao.get_tracks(get_user_id(),
                                     itinerary_id, features.tracks);
  time_span_type time_span = ItineraryPgDao::get_time_span(tracks, waypoints);
  auto bounding_box = ItineraryPgDao::get_bounding_box(tracks, routes, waypoints);
  xml_document doc;
  doc.load_string("<kml xmlns=\"http://www.opengis.net/kml/2.2\" "
                  "xmlns:gx=\"http://www.google.com/kml/ext/2.2\">");
  xml_node decl = doc.prepend_child(node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";
  xml_node kml = doc.last_child().append_child("Document");
  kml.append_child("name").append_child(node_pcdata).set_value("GPS device");
  std::time_t now_t = now.time_t();
  std::ostringstream ss;
  ss << "Created " << std::put_time(gmtime(&now_t), "%a %b %d %H:%M:%S %Y");
  kml.append_child("snippet").append_child(node_pcdata)
    .set_value(ss.str().c_str());
  xml_node look_at = kml.append_child("LookAt");
  if (time_span.is_valid) {
    DateTime begin(time_span.start);
    DateTime end(time_span.finish);
    xml_node time_span = look_at.append_child("gx:TimeSpan");
    time_span.append_child("begin").append_child(node_pcdata)
      .set_value(begin.get_time_as_iso8601_gmt().c_str());
    time_span.append_child("end").append_child(node_pcdata)
      .set_value(end.get_time_as_iso8601_gmt().c_str());
  }

  const auto center = bounding_box.first
    ? bounding_box.second.get_center()
    : location();
  double range = 20000.0; // Default distance in meters
  if (bounding_box.first) {
    // distance in kilometers
    double diagonal_distance =
      GeoUtils::distance(bounding_box.second.top_left,
                         bounding_box.second.bottom_right);
    if (diagonal_distance < 1)
      diagonal_distance = 1;
    // Calculate range value in meters
    range = diagonal_distance * 1300.0;
  }
  look_at.append_child("longitude").append_child(node_pcdata)
    .set_value(std::to_string(center.longitude).c_str());
  look_at.append_child("latitude").append_child(node_pcdata)
    .set_value(std::to_string(center.latitude).c_str());
  look_at.append_child("range").append_child(node_pcdata)
    .set_value(std::to_string(range).c_str());

  if (!routes.empty())
    append_kml_route_styles(kml);
  if (!tracks.empty())
    append_kml_track_styles(kml);
  append_kml_styles(kml);

  if (!routes.empty()) {
    auto style = kml.append_child("Style");
    style.append_attribute("id").set_value("lineStyle");
    auto lineStyle = style.append_child("LineStyle");
    lineStyle.append_child("color").append_child(node_pcdata)
      .set_value("99ffac59");
    lineStyle.append_child("width").append_child(node_pcdata)
      .set_value("6");
  }

  append_kml_waypoints(kml, waypoints);
  append_kml_tracks(kml, tracks);
  append_kml_routes(kml, routes);

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
  response.set_header("Content-Type", get_mime_type(download_type == gpx ? "gpx" : "kml"));
  std::time_t now = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());
  std::tm tm = *std::localtime(&now);
  std::ostringstream filename;
  filename << "trip-itinerary-" << itinerary_id << (download_type == gpx ? ".gpx" : ".kml");
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
    const auto action = request.get_param("action");
    itinerary_id = std::stol(request.get_param("id"));
    if (action == "download-kml") {
      download_type = kml;
      handle_kml_download(request, response);
    } else {
      download_type = gpx;
      handle_gpx_download(request, response);
    }
  } catch (const std::exception &e) {
    std::cerr << "Exception downloading itinerary: "
              << e.what() << '\n';
  }
}
