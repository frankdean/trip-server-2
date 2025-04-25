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
#include "itinerary_track_segment_edit_handler.hpp"
#include "geo_utils.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/pagination.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <syslog.h>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

void ItineraryTrackSegmentEditHandler::build_form(
    std::ostream &os,
    const Pagination& pagination,
    const ItineraryPgDao::track_segment &segment)
{
  os
    <<
    "<div class=\"container-fluid\">\n"
    "  <div class=\"page-header\">\n"
    "    <h1>" << get_page_title() << "</h1>\n"
    "  </div>\n"
    // Label showing the ID of a itinerary track segment on the page listing its points
    "  <p>" << format(translate("<strong>Segment ID:</strong>&nbsp;{1,number=left}")) % segment_id << "</p>\n";
  os <<  "  <form method=\"post\">\n"
    "      <input type=\"hidden\" name=\"itineraryId\" value=\"" << itinerary_id << "\">\n"
    "      <input type=\"hidden\" name=\"track-page\" value=\"" << track_page << "\">\n"
    "      <input type=\"hidden\" name=\"trackId\" value=\"" << track_id << "\">\n"
    "      <input type=\"hidden\" name=\"segmentId\" value=\"" << segment_id << "\">\n"
    "      <input type=\"hidden\" name=\"shared\" value=\"" << (read_only ? "true" : "false") << "\">\n"
    "      <input type=\"hidden\" name=\"active-tab\" value=\"features\">\n";
  if (segment.points.empty()) {
    os
      <<
      "    <div id=\"empty-segment\" class=\"alert alert-info\">\n"
      // Information alert shown when a track segment has no points
      "      <p>" << translate("Track segment has no points") << "</p>\n"
      "    </div>\n";
  } else {
    os
      <<
      "    <div>\n"
      "      <p>\n";
    if (segment.distance.has_value()) {
      const auto miles = segment.distance.value() / kms_per_mile;
      os
        <<
        // Displays segment distance in kilometers
        "        <span>" << format(translate("{1,num=fixed,precision=2} km", "{1,num=fixed,precision=2} kms", segment.distance.value())) % segment.distance.value() << "</span>\n"
        // Displays segment distance in miles
        "        <span>"  << format(translate("{1,num=fixed,precision=2} mile", "{1,num=fixed,precision=2} miles", miles)) % miles << "</span>\n";
    }
    if (segment.ascent.has_value())
      // Displays segment ascent in meters
      os << "        <span> ↗︎" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % segment.ascent.value() << "</span>\n";
    if (segment.descent.has_value())
      // Displays segment descent in meters
      os << "        <span> ↘︎" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % segment.descent.value() << "</span>\n";
    if (segment.ascent.has_value())
      // Displays segment ascent in feet
      os << "        <span> ↗︎" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (segment.ascent.value() / inches_per_meter / 12) << "</span>\n";
    if (segment.descent.has_value())
      // Displays segment descent in feet
      os << "        <span> ↘︎" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (segment.descent.value() / inches_per_meter / 12) << "</span>\n";
    if (segment.highest.has_value() && segment.lowest.has_value()) {
      os <<
        // Displays segment highest point in meters
        "        <span> " << format(translate("{1,num=fixed,precision=0}")) % segment.highest.value()
        // Displays segment lowest point in meters
         << "⇅" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % segment.lowest.value() << "</span>\n"
        // Displays segment highest point in feet
        "        <span> " << format(translate("{1,num=fixed,precision=0}")) % (segment.highest.value() / inches_per_meter / 12)
        // Displays segment lowest point in feet
         << "&#x21c5;" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (segment.lowest.value() / inches_per_meter / 12) << "</span>\n";
    }
    os
      <<
      "      </p>\n"
      "      <table id=\"selection-list\" class=\"table table-striped\">\n"
      "        <tr>\n"
      "          <th class=\"text-start\"><input id=\"input-select-all\" type=\"checkbox\" name=\"select-all\" accesskey=\"a\"";
    if (select_all)
      os << " checked";
    os <<
      ">\n"
      // Column heading for track segment point IDs
      "            <label for=\"input-select-all\">" << translate("ID") << "</label>\n"
      "          </th>\n"
      // Column heading for track segment point times
      "          <th class=\"text-start\">" << translate("Time") << "</th>\n"
      // Column heading for track segment point latitudes
      "          <th class=\"text-end\">" << translate("Latitude") << "</th>\n"
      // Column heading for track segment point longitudes
      "          <th class=\"text-end\">" << translate("Longitude") << "</th>\n"
      // Column heading for track segment point altitudes
      "          <th class=\"text-end\">" << translate("Altitude") << "</th>\n"
      // Column heading for track segment point HDOPs
      "          <th class=\"text-end\">" << translate("HDOP") << "</th>\n"
      "        </tr>\n";
    for (const auto &point : segment.points) {
      if (point.id.has_value()) {
        std::ostringstream label;
        label << "select-point-" << point.id.value();
        os <<
          "        <tr>\n"
          "          <td class=\"text-start\"><input id=\"" << label.str() << "\" type=\"checkbox\" name=\"point[" << point.id.value() << "]\" value=\"" << point.id.value() << "\"";
        if (select_all || selected_point_id_map.find(point.id.value()) != selected_point_id_map.end())
          os << " checked";
        os << ">\n"
          "            <label for=\"" << label.str() << "\">" << as::number << std::setprecision(0) << point.id.value() << as::posix << "</label>\n"
          "          </td>\n"
          "          <td class=\"text-start\">";
        if (point.time.has_value()) {
          const auto date = std::chrono::duration_cast<std::chrono::seconds>(
              point.time.value().time_since_epoch()
            ).count();
            // os << as::ftime("%a") << date << " "
          os << as::date_medium << as::datetime << date << as::posix;
        }
        os << "</td>\n"
          "          <td class=\"text-end\">" << std::fixed << std::setprecision(6) << point.latitude << "</td>\n"
          "          <td class=\"text-end\">" << point.longitude << "</td>\n"
          "          <td class=\"text-end\">";
        if (point.altitude.has_value())
          os << std::fixed << std::setprecision(0) << point.altitude.value();
        os <<
          "</td>\n"
          "          <td class=\"text-end\">";
        if (point.hdop.has_value())
          os << as::number << std::fixed << std::setprecision(1) << point.hdop.value() << as::posix;
        os << "</td>\n"
          "        </tr>\n";
      }
    }
    os <<
      "      </table>\n";

    const auto page_count = pagination.get_page_count();
    if (page_count > 1) {
      os
        <<
        "    <div id=\"div-paging\" class=\"pb-0\">\n"
        << pagination.get_html()
        <<
        "    </div>\n"
        "    <div class=\"d-flex justify-content-center pt-0 pb-0 col-12\">\n"
        "      <input id=\"page\" type=\"number\" name=\"page\" value=\""
        << std::fixed << std::setprecision(0) << pagination.get_current_page()
        << "\" min=\"1\" max=\"" << page_count << "\">\n"
        // Title of button which goes to a specified page number
        "      <button id=\"goto-page-btn\" class=\"btn btn-sm btn-primary\" type=\"submit\" name=\"action\" accesskey=\"g\" value=\"page\">" << translate("Go") << "</button>\n"
        "    </div>\n"
        ;
    }
    os <<
      "    </div><!-- starts after form -->\n";
  }
  os <<
    "    <div id=\"div-buttons\">\n";
  if (!read_only && !segment.points.empty()) {
    os <<
      // Label for button to delete one or more selected points from a track segment
      "      <button id=\"btn-delete\" name=\"action\" value=\"delete\" accesskey=\"d\" class=\"my-1 btn btn-lg btn-danger\" onclick=\"return confirm('" << translate("Delete the selected points?") << "');\">"
      // Button label for deleting a seletion of points in a track segment
       << translate("Delete points") << "</button>\n"
      // Label for button to split a track segment by selected point
      "      <button id=\"btn-split\" name=\"action\" value=\"split\" accesskey=\"s\" class=\"my-1 btn btn-lg btn-danger\" onclick=\"return confirm('" << translate("Split segment before selected point?") << "');\">"
      // Button label to split a track segment at a selected point
       << translate("Split segment") << "</button>\n";
  }
  os <<
    // Label for button to return to the itinerary track when viewing a list of track segment points
    "        <button id=\"btn-close\" accesskey=\"c\" formmethod=\"get\" formaction=\"" << get_uri_prefix() << "/itinerary-track-edit\" class=\"my-1 btn btn-lg btn-danger\">" << translate("Close") << "</button>\n"
    "    </div>\n"
    "  </form>\n"
    "</div>\n";
  if (!segment.points.empty()) {
    os <<
      "<div id=\"itinerary-track-map\"></div>\n";
    json features;
    // features["tracks"] = {track_id};
    // features["routes"] = json::array();
    // features["waypoints"] = json::array();
    json points;
    for (const auto &point : segment.points) {
      if (point.id.has_value())
        points.push_back(point.id.value());
    }
    // features["points"] = points;
    json j{
      {"itinerary_id", itinerary_id},
      {"track_id", track_id},
      // {"segment_id", segment_id},
      {"track_point_ids", points}
    };
    // std::cout << "pageInfoJSON:\n" << j.dump(4) << '\n';
    os <<
      "<script>\n"
      "<!--\n"
      "const pageInfoJSON = '" << j << "';\n"
      "const server_prefix = '" << get_uri_prefix() << "';\n";
    append_map_provider_configuration(os);
    os <<
      "// -->\n"
      "</script>\n";
  }
}

void ItineraryTrackSegmentEditHandler::do_preview_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  (void)request; // unused
  (void)response;
  itinerary_id = std::stol(request.get_param("itineraryId"));
  track_id = std::stol(request.get_param("trackId"));
  segment_id = std::stol(request.get_param("segmentId"));
  const std::string shared = request.get_param("shared");
  read_only = shared == "true";
  // page title of the itinerary track segment editing page
  set_page_title(translate("Itinerary Track Segment Edit"));
  set_menu_item(unknown);
  if (request.method == HTTPMethod::post) {
    // set `select_all` depending on HTTPMethod as in some POST situations the
    // get parameter may also be set conflictingly
    select_all = "on" == request.get_post_param("select-all");
    action = request.get_param("action");
    if (!action.empty()) {
      // std::cout << "Action: \"" << action << "\"\n";
      selected_point_id_map = request.extract_array_param_map("point");
      for (const auto &m : selected_point_id_map) {
        if (!m.second.empty())
          selected_point_ids.push_back(m.first);
      }
    }
  } else if (request.method == HTTPMethod::get) {
    select_all = "on" == request.get_query_param("select-all");
  } else {
    select_all = "on" == request.get_param("select-all");
  }
}

void ItineraryTrackSegmentEditHandler::delete_points(
    const HTTPServerRequest& request,
    ItineraryPgDao &dao)
{
  (void)request; // unused
  bool dirty = false;
  auto track = dao.get_track(get_user_id(), itinerary_id, track_id);
  int segment_index = -1;
  for (auto it = track.segments.begin(); it != track.segments.end(); it++) {
    if (it->id.has_value() && it->id.value() == segment_id) {
      segment_index = std::distance(track.segments.begin(), it);
      it->points.erase(
          std::remove_if(
              it->points.begin(),
              it->points.end(),
          [&](const ItineraryPgDao::track_point &point) {
            bool retval = point.id.has_value() &&
              selected_point_id_map.find(point.id.value()) !=
              selected_point_id_map.end();
            if (retval)
              dirty = true;
            return retval;
          }),
          it->points.end());
    }
  }
  if (dirty) {
    track.calculate_statistics();
    dao.save(
        get_user_id(),
        itinerary_id,
        track);
    try {
      auto segment = track.segments.at(segment_index);
      assert(segment.id.has_value());
      segment_id = segment.id.value();
    } catch (const std::out_of_range &e) {
      std::cerr << "Error finding segment after deleting points: "
                << e.what() << '\n';
      syslog(LOG_ERR,
             "Error finding segment after deleting points: %s",
             e.what());
    }
  }
}

void ItineraryTrackSegmentEditHandler::split_segment(
    const HTTPServerRequest& request,
    ItineraryPgDao &dao)
{
  (void)request; // unused
  long split_before_id = selected_point_ids.front();
  // std::cout << "Splitting segment at point ID: " << split_before_id << '\n';
  auto track = dao.get_track(get_user_id(), itinerary_id, track_id);
  ItineraryPgDao::track_segment new_segment;
  // Insert a new segment after the current segment
  long find_id = segment_id;
  auto current_segment = std::find_if(
      track.segments.begin(),
      track.segments.end(),
      [find_id](const ItineraryPgDao::track_segment &segment) {
        return segment.id.has_value() && segment.id.value() == find_id;
      }
    );
  if (current_segment != track.segments.end()) {
    // Add all the points that are greater than or equal to the split point to the new segment
    for (auto i = current_segment->points.begin(); i != current_segment->points.end(); i++)
      if (i->id.has_value() && i->id.value() >= split_before_id)
        new_segment.points.push_back(*i);
    int count = 0;
    // Remove those same points from the current segment
    current_segment->points.erase(
        std::remove_if(
            current_segment->points.begin(),
            current_segment->points.end(),
            [split_before_id, &count](const ItineraryPgDao::track_point &point) {
              bool retval = point.id.has_value() && point.id.value() >= split_before_id;
              if (retval) {
                // std::cout << "Removing point " << point.id.second << '\n';
                count++;
                // } else {
                // std::cout << "Skipping point " << point.id.second << '\n';
              }
              return retval;
            }),
        current_segment->points.end());
    current_segment++;
    if (current_segment != track.segments.end())
      track.segments.insert(current_segment, new_segment);
    else
      track.segments.push_back(new_segment);
    track.calculate_statistics();
    dao.save(get_user_id(), itinerary_id, track);
  } else {
    std::cerr << "Failed to find a segment with id " << segment_id << " in current track\n";
    syslog(LOG_ERR, "Failed to find a segment with id %ld in current track", segment_id);
  }
}

void ItineraryTrackSegmentEditHandler::append_pre_body_end(
    std::ostream& os) const
{
  BaseMapHandler::append_pre_body_end(os);
  os << "    <script type=\"module\" src=\"" << get_uri_prefix() << "/static/js/itinerary-segment-edit.js\"></script>\n";
}

void ItineraryTrackSegmentEditHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  track_page = request.get_param("track-page");
  ItineraryPgDao dao;
  if (action == "delete") {
    delete_points(request, dao);
  } else if (action == "split" && !selected_point_ids.empty()) {
    split_segment(request, dao);
    std::ostringstream url;
    url << get_uri_prefix() << "/itinerary-track-edit?itineraryId=" << itinerary_id
        << "&trackId=" << track_id
        << "&track-page=" << track_page
        << "&select-all="<< (select_all ? "on" : "")
        << "&active-tab=features";
    redirect(request, response, url.str());
    return;
  }
  std::map<std::string, std::string> page_param_map;
  page_param_map["itineraryId"] = std::to_string(itinerary_id);
  page_param_map["trackId"] = std::to_string(track_id);
  page_param_map["segmentId"] = std::to_string(segment_id);
  page_param_map["track-page"] = track_page;
  page_param_map["shared"] = read_only ? "true" : "false";
  page_param_map["select-all"] = select_all ? "on" : "";
  const long total_count = dao.get_track_segment_point_count(get_user_id(),
                                                             itinerary_id,
                                                             segment_id);
  Pagination pagination(get_uri_prefix() + "/itinerary-track-segment-edit",
                        page_param_map,
                        total_count);
  const std::string page = request.get_param("page");
  try {
    if (!page.empty())
      pagination.set_current_page(std::stoul(page));
  } catch (const std::logic_error& e) {
    std::cerr << "Error converting string to page number\n";
  }
  auto segment = dao.get_track_segment(get_user_id(),
                                       itinerary_id,
                                       segment_id,
                                       pagination.get_offset(),
                                       pagination.get_limit());
  build_form(response.content, pagination, segment);
}
