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
#include "itinerary_upload_handler.hpp"
#ifdef HAVE_GDAL
#include "elevation_tile.hpp"
#endif
#include "../trip-server-common/src/get_options.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using namespace boost::locale;
using namespace pugi;

#ifdef HAVE_GDAL
extern ElevationService *elevation_service;
#endif

/**
 * Fetches an XML node element's value as an optional pair containing a
 * string.
 * \param node the parent node of the element to fetch
 * \param child_name the element name of the child's value to fetch
 * \return a std::pair<bool, std::string>, true indicating the element exists
 */
std::pair<bool, std::string>
    ItineraryUploadHandler::child_node_as_string(
        const pugi::xml_node &node, const pugi::char_t *child_name)
{
  auto t = node.child(child_name);
  std::pair<bool, std::string> retval;
  if ((retval.first = t.type() != pugi::node_null))
    retval.second = t.child_value();
  // std::cout << "child_node of node: " << node.name() << "> "
  //           << " => " << child_name << " \""
  //           << (retval.first ? retval.second : "[null]") << "\"\n";
  return retval;
}

/**
 * Fetches an XML node element's value as an optional pair containing a
 * double.
 * \param node the parent node of the element to fetch
 * \param child_name the element name of the child's value to fetch
 * \return a std::pair<bool, double>, true indicating the element exists.
 * It is set to false if a numeric conversion exception occurs.
 */
std::pair<bool, double> ItineraryUploadHandler::child_node_as_double(
    const pugi::xml_node &node, const pugi::char_t *child_name)
{
  auto t = node.child(child_name);
  std::pair<bool, double> retval;
  try {
    if ((retval.first = t.type() != pugi::node_null))
      retval.second = std::stod(t.child_value());
  } catch (const std::invalid_argument &e) {
    retval.first = false;
  } catch (const std::out_of_range &e) {
    retval.first = false;
  }
  return retval;
}

/**
 * Fetches an XML node element's value as an optional pair containing a
 * float.
 * \param node the parent node of the element to fetch
 * \param child_name the element name of the child's value to fetch
 * \return a std::pair<bool, float>, true indicating the element exists.
 * It is set to false if a numeric conversion exception occurs.
 */
std::pair<bool, float> ItineraryUploadHandler::child_node_as_float(
    const pugi::xml_node &node, const pugi::char_t *child_name)
{
  auto t = node.child(child_name);
  std::pair<bool, float> retval;
  try {
    if ((retval.first = t.type() != pugi::node_null))
      retval.second = std::stod(t.child_value());
  } catch (const std::invalid_argument &e) {
    retval.first = false;
  } catch (const std::out_of_range &e) {
    retval.first = false;
  }
  return retval;
}

/**
 * Fetches an XML node element's value as an optional pair containing a
 * long.
 * \param node the parent node of the element to fetch
 * \param child_name the element name of the child's value to fetch
 * \return a std::pair<bool, long>, true indicating the element exists.
 * It is set to false if a numeric conversion exception occurs.
 */
std::pair<bool, long> ItineraryUploadHandler::child_node_as_long(
    const pugi::xml_node &node, const pugi::char_t *child_name)
{
  auto t = node.child(child_name);
  std::pair<bool, long> retval;
  try {
    if ((retval.first = t.type() != pugi::node_null))
      retval.second = std::stol(t.child_value());
  } catch (const std::invalid_argument &e) {
    retval.first = false;
  } catch (const std::out_of_range &e) {
    retval.first = false;
  }
  return retval;
}

/**
 * Fetches an XML node element's value as an optional pair containing a
 * time_point.
 * \param node the parent node of the element to fetch
 * \param child_name the element name of the child's value to fetch
 * \return a std::pair<bool, std::chrono::system_clock::time_point>, true
 * indicating the element exists
 */
std::pair<bool, std::chrono::system_clock::time_point>
    ItineraryUploadHandler::child_node_as_time_point(const pugi::xml_node &node,
                                                     const pugi::char_t *child_name)
{
  auto t = node.child(child_name);
  std::pair<bool, std::chrono::system_clock::time_point> retval;
  if ((retval.first = t.type() != pugi::node_null)) {
    utils::DateTime time(t.child_value());
    retval.second = time.time_tp();
  }
  return retval;
}

void ItineraryUploadHandler::build_form(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  response.content
    <<
    "  <div class=\"container-fluid bg-light my-3\">\n"
    // Title of the page for uploading a GPX (XML) file containing routes, waypoints and tracks
    "    <h1 class=\"pt-2\">" << translate("Itinerary Upload") << "</h1>\n"
    "    <form id=\"form\" enctype=\"multipart/form-data\" method=\"post\">\n"
    "      <div class=\"col-lg-6\">\n"
    // Instructions for uploading a GPX (XML) file containing routes, waypoints and tracks
    "        <p>" << translate("Select the GPX file to be uploaded, then click the upload button.") << "</p>\n"
    "        <input type=\"hidden\" name=\"id\" value=\"" << itinerary_id << "\">\n"
    "        <input id=\"btn-file-upload\" type=\"file\" accesskey=\"f\" name=\"file\" class=\"btn btn-lg btn-primary\">\n"
    "      </div>\n"
    "      <div class=\"col-12 py-3\" arial-label=\"Form buttons\">\n"
    // Label for button to upload a GPX (XML) file containing routes, waypoints and tracks
    "        <button id=\"btn-upload\" type=\"submit\" accesskey=\"u\" name=\"action\" value=\"upload\" class=\"btn btn-lg btn-success\">" << translate("Upload") << "</button>\n"
    // Label for button to cancel uploading a GPX (XML) file containing routes, waypoints and tracks
    "        <button id=\"btn-cancel\" type=\"submit\" accesskey=\"c\" name=\"action\" value=\"cancel\" class=\"btn btn-lg btn-danger\" formnovalidate>" << translate("Cancel") << "</button>\n"
    "      </div>\n"
    "    </form>\n"
    "  </div>\n";
}

void ItineraryUploadHandler::do_preview_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  set_page_title(translate("Itinerary Upload"));
  try {
    itinerary_id = std::stol(request.get_param("id"));
  } catch (const std::exception &e) {
    std::cerr << "Couldn't get value for itinerary id\n";
  }
  // set_menu_item(unknown);
}

void ItineraryUploadHandler::add_waypoint(
    ItineraryPgDao::itinerary_features &features,
    const pugi::xml_node &node)
{
  ItineraryPgDao::waypoint wpt;
  wpt.longitude = node.attribute("lon").as_double();
  wpt.latitude = node.attribute("lat").as_double();
  wpt.altitude = child_node_as_double(node, "ele");
  wpt.time = child_node_as_time_point(node, "time");
  wpt.name = child_node_as_string(node, "name");
  wpt.comment = child_node_as_string(node, "cmt");
  wpt.symbol = child_node_as_string(node, "sym");
  wpt.description = child_node_as_string(node, "desc");
  wpt.type = child_node_as_string(node, "type");
  auto exts = node.child("extensions");
  if (exts.type() != node_null) {
    wpt.color = child_node_as_string(exts, "color");
    auto ext = exts.child("wptx1:WaypointExtension");
    if (ext.type() != node_null)
      wpt.avg_samples = child_node_as_long(ext, "wptx1:Samples");
  }
  features.waypoints.push_back(wpt);
}

void ItineraryUploadHandler::add_route_point(
    ItineraryPgDao::route &route,
    const pugi::xml_node &node)
{
  ItineraryPgDao::route_point p;
  p.longitude = node.attribute("lon").as_double();
  p.latitude = node.attribute("lat").as_double();
  p.altitude = child_node_as_double(node, "ele");
  p.name = child_node_as_string(node, "name");
  p.comment = child_node_as_string(node, "cmt");
  p.description = child_node_as_string(node, "desc");
  p.symbol = child_node_as_string(node, "sym");
  route.points.push_back(p);
}

void ItineraryUploadHandler::add_route(
    ItineraryPgDao::itinerary_features &features,
    const pugi::xml_node &node)
{
  ItineraryPgDao::route rt;
  rt.name = child_node_as_string(node, "name");
  auto exts = node.child("extensions");
  if (exts.type() != node_null) {
    auto ext = exts.child("gpxx:RouteExtension");
    if (ext.type() != node_null)
      rt.color_key = child_node_as_string(ext, "gpxx:DisplayColor");
  }
  for (xml_node n : node.children()) {
    const std::string name = n.name();
    // std::cout << "Node: \"" << name << "\"\n";
    if (name == "rtept")
      add_route_point(rt, n);
  }
  features.routes.push_back(rt);
}

void ItineraryUploadHandler::add_track_point(
    ItineraryPgDao::track_segment &track_segment,
    const pugi::xml_node &node)
{
  ItineraryPgDao::track_point p;
  p.longitude = node.attribute("lon").as_double();
  p.latitude = node.attribute("lat").as_double();
  p.altitude = child_node_as_double(node, "ele");
  p.time = child_node_as_time_point(node, "time");
  p.hdop = child_node_as_float(node, "hdop");
  track_segment.points.push_back(p);
}

void ItineraryUploadHandler::add_track_segment(
    ItineraryPgDao::track &track,
    const pugi::xml_node &node)
{
  ItineraryPgDao::track_segment trkseg;
  for (xml_node n : node.children()) {
    const std::string name = n.name();
    // std::cout << "Node: \"" << name << "\"\n";
    if (name == "trkpt")
      add_track_point(trkseg, n);
  }
  track.segments.push_back(trkseg);
}

void ItineraryUploadHandler::add_track(
    ItineraryPgDao::itinerary_features &features,
    const pugi::xml_node &node)
{
  ItineraryPgDao::track trk;
  trk.name = child_node_as_string(node, "name");
  auto exts = node.child("extensions");
  if (exts.type() != node_null) {
    auto ext = exts.child("gpxx:TrackExtension");
    if (ext.type() != node_null)
      trk.color_key = child_node_as_string(ext, "gpxx:DisplayColor");
  }
  for (xml_node n : node.children()) {
    const std::string name = n.name();
    // std::cout << "Node: \"" << name << "\"\n";
    if (name == "trkseg")
      add_track_segment(trk, n);
  }
  features.tracks.push_back(trk);
}

void ItineraryUploadHandler::save(const xml_document &doc)
{
  ItineraryPgDao::itinerary_features features;
  xml_node gpx = doc.child("gpx");
  // Waypoints
  for (xml_node n : gpx.children()) {
    const std::string name = n.name();
    // std::cout << "Node: \"" << name << "\"\n";
    if (name == "wpt") {
      add_waypoint(features, n);
    } else if (name == "rte") {
      add_route(features, n);
    } else if (name == "trk") {
      add_track(features, n);
    }
  }
#ifdef HAVE_GDAL
  if (elevation_service) {
    elevation_service->fill_elevations_for_paths(
        features.routes.begin(),
        features.routes.end());

    for (auto &i : features.tracks) {
      elevation_service->fill_elevations_for_paths(
          i.segments.begin(),
          i.segments.end());
    }
    elevation_service->fill_elevations(
        features.waypoints.begin(),
        features.waypoints.end());
  } else if (GetOptions::verbose_flag) {
    std::cerr << "Elevation service is not available\n";
  }
#endif // HAVE_GDAL
  ItineraryPgDao::route::calculate_statistics(features.routes);
  ItineraryPgDao::track::calculate_statistics(features.tracks);
  ItineraryPgDao dao;
  dao.create_itinerary_features(get_user_id(), itinerary_id, features);
}

void ItineraryUploadHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  // auto pp = request.get_post_params();
  // for (auto const &p : pp) {
  //   std::cout << "param: \"" << p.first << "\" -> \"" << p.second << "\"\n";
  // }
  // for (auto const &h : request.headers) {
  //   std::cout << "header: \"" << h.first << "\" -> \"" << h.second << "\"\n";
  // }
  const std::string action = request.get_post_param("action");
  // std::cout << "Action: \"" << action << "\"\n";

  try {
    // std::cout << "Upload handler for itinerary ID: " << itinerary_id << "\n";
    if (action.empty() && request.method == HTTPMethod::get) {
      build_form(request, response);
      return;
    } else if (action == "upload") {
      // const std::string filename = request.get_post_param("file");
      const auto multiparts = request.multiparts;
      try {
        const auto file_data = multiparts.at("file");
        xml_document doc;
        xml_parse_result result = doc.load_string(file_data.body.c_str());
        // std::cout << file_data.body << '\n';
        if (result) {
          // std::cout << "XML parsed without errors\n";
          save(doc);
        } else {
          std::cerr << "Error parsing XML: " << result.description() << "\n"
                    << "Error offset: " << result.offset << '\n';
        }
      } catch (const std::out_of_range &e) {
        std::cerr << "No file uploaded\n";
      }
    } else if (action != "cancel") {
      throw BadRequestException("Invalid GPX upload request");
    }
    redirect(request, response,
             get_uri_prefix() + "/itinerary?id=" + std::to_string(itinerary_id) + "&active-tab=features");
  } catch (const std::exception &e) {
    std::cerr << "Exception handling GPX upload request: "
              << e.what() << '\n';
    throw;
  }
}
