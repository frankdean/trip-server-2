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
#include "itinerary_track_edit_handler.hpp"
#include "geo_utils.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/pagination.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

void ItineraryTrackEditHandler::build_form(
    std::ostream &os,
    const Pagination& pagination,
    const ItineraryPgDao::track &track)
{
  os
    <<
    "<div class=\"container-fluid\">\n"
    "  <h1>" << get_page_title() << "</h1>\n"
    "  <form name=\"form\" method=\"post\">\n"
    // "    <div class=\"container-fluid bg-light row g-3 my-3 pb-3 mx-0\">\n"
    "    <input type=\"hidden\" name=\"id\" value=\"" << itinerary_id << "\">\n"
    "    <input type=\"hidden\" name=\"itineraryId\" value=\"" << itinerary_id << "\">\n"
    "    <input type=\"hidden\" name=\"trackId\" value=\"" << track.id.second << "\">\n"
    "    <input type=\"hidden\" name=\"shared\" value=\"" << (read_only ? "true" : "false") << "\">\n"
    "    <input type=\"hidden\" name=\"active-tab\" value=\"features\">\n";
  if (track.segments.empty()) {
    os
      <<
      "  <div id=\"track-not-found\" class=\"alert alert-info\">\n"
      // Information alert shown when a track has no segments
      "    <p>" << translate("Track has no segments") << "</p>\n"
      "  </div>\n";
  } else {
    os
      <<
      "      <div>";
    if (track.name.first) {
      // Formatted output of label and track name
      os << format(translate("<strong>Name:</strong>&nbsp;{1}")) % x(track.name.second);
    } else if (track.id.first) {
      // Database ID of an item, typically a route, track or waypoint
      os << format(translate("<strong>ID:</strong>&nbsp;{1,number=left}")) % track.id.second;
    }
    os
      << "</div>\n";
    if (track.color_description.first) {
      // Formatted output of label and track color
      os << "    <div>" << format(translate("<strong>Color:</strong>&nbsp;{1}")) % x(track.color_description.second) << "</div>\n";
    }
    os << "      <div class=\"mb-3\">\n";
    if (track.distance.first) {
      // Shows the total distance for a route or track in kilometers
      os
        << "        <span>&nbsp;" << format(translate("{1,num=fixed,precision=2}&nbsp;km")) % track.distance.second
        // Shows the total distance for a route or track in miles
        << "&nbsp;" << format(translate("{1,num=fixed,precision=2}&nbsp;mi")) % (track.distance.second / kms_per_mile) << "</span>\n";
    }
    if (track.ascent.first) {
      // Shows the total ascent for a route or track in meters
      os << "        <span>&nbsp;↗︎" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % track.ascent.second << "</span>\n";
    }
    if (track.descent.first) {
      // Shows the total descent for a route or track in kilometers
      os << "        <span>&nbsp;↘" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % track.descent.second << "</span>\n";
    }
    if (track.ascent.first) {
      // Shows the total ascent for a route or track in feet
      os << "        <span>&nbsp;↗︎" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (track.ascent.second / inches_per_meter / 12) << "</span>\n";
    }
    if (track.descent.first) {
      // Shows the total descent for a route or track in feet
      os << "        <span>&nbsp;↘" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (track.descent.second / inches_per_meter / 12) << "</span>\n";
    }
    if (track.highest.first) {
      os
        // Shows the highest point of a route or track in meters
        << "        <span>&nbsp; " << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % track.highest.second << "</span>";
    }
    if (track.highest.first || track.lowest.first)
      os << " ⇅";
    if (track.lowest.first) {
      os
        // Shows the lowest point of a route or track in meters
        << "        <span>" << format(translate("{1,num=fixed,precision=0}&nbsp;m")) % track.lowest.second << "</span>\n";
    }
    if (track.highest.first) {
      os
        // Shows the highest point of a route or track in meters
        << "        <span>&nbsp;" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (track.highest.second / inches_per_meter / 12) << "</span>";
    }
    if (track.highest.first || track.lowest.first)
      os << " &#x21c5;";
    if (track.lowest.first) {
      os
        // Shows the lowest point of a route or track in meters
        << "        <span>" << format(translate("{1,num=fixed,precision=0}&nbsp;ft")) % (track.lowest.second / inches_per_meter / 12) << "</span>\n";
    }
    os
      <<
      "      </div>\n"
      "      <table id=\"selection-list\" class=\"table table-striped\">\n"
      "        <tr>\n"
      "          <th style=\"width: 60px;\"><input id=\"input-select-all\" type=\"checkbox\" name=\"select-all\" accesskey=\"a\"";
    if (select_all)
      os << " checked";
    os <<
      "></th>\n"
      // Column heading showing track segment IDs
      "          <th>" << translate("Segment ID") << "</th>\n"
      "        </tr>\n";
    for (const auto &segment : track.segments) {
      os <<
        "        <tr>\n"
        "          <td>\n"
        "            <input type=\"checkbox\" name=\"segment[" << segment.id.second << "]\" value=\"" << segment.id.second << "\"";
      if (select_all || selected_segment_id_map.find(segment.id.second) != selected_segment_id_map.end())
        os << " checked";
      os <<
        ">\n"
        "          </td>\n"
        "          <td><a href=\"" << get_uri_prefix()
         << "/itinerary-track-segment-edit?itineraryId=" << itinerary_id
         << "&trackId=" << track.id.second
         << "&segmentId=" << segment.id.second
         << "&track-page=" << pagination.get_current_page()
         << "&select-all=" << (select_all ? "on" : "")
         << "&shared=" << (read_only ? "true" : "false")
         << "\">" << as::number << std::setprecision(0) << segment.id.second
         << as::posix << "</a></td>\n"
        "        </tr>\n";
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
        "      <input id=\"page\" type=\"number\" name=\"track-page\" value=\""
        << std::fixed << std::setprecision(0) << pagination.get_current_page()
        << "\" min=\"1\" max=\"" << page_count << "\">\n"
        // Title of button which goes to a specified page number
        "      <button id=\"goto-page-btn\" class=\"btn btn-sm btn-primary\" type=\"submit\" name=\"action\" accesskey=\"g\" value=\"page\">" << translate("Go") << "</button>\n"
        "    </div>\n"
        ;
    }
  }
  os <<
    "      <div id=\"div-buttons\">\n";
  if (!read_only && !track.segments.empty()) {
    os <<
      // Confirmation message to delete one or more selected segments from a track
      "        <button id=\"btn-delete\" name=\"action\" value=\"delete\" accesskey=\"d\" class=\"my-1 btn btn-lg btn-danger\" onclick=\"return confirm('" << translate("Delete the selected segments?") << "');\">"
      // Button label for deleting a selection of track segments from a track
       << translate("Delete segments") << "</button>\n"
      // Confirmation message to split a track by selected segment
      "        <button id=\"btn-split\" name=\"action\" value=\"split\" accesskey=\"s\" class=\"my-1 btn btn-lg btn-danger\" onclick=\"return confirm('" << translate("Split track before selected segment?") << "');\">"
      // Button label for splitting a track at a selected segment
       << translate("Split track") << "</button>\n"
      // Confirmation message to merge one or more segments
      "        <button id=\"btn-merge\" name=\"action\" value=\"merge\" accesskey=\"m\" class=\"my-1 btn btn-lg btn-danger\" onclick=\"return confirm('" << translate("Merge selected segments?") << "');\">"
      // Button label to merge one or more segments
       << translate("Merge segments") << "</button>\n";
  }
  os <<
      // Label for button to return to the itinerary when viewing a list of track segments
    "        <button id=\"btn-close\" accesskey=\"c\" formmethod=\"get\" formaction=\"" << get_uri_prefix() << "/itinerary\" class=\"my-1 btn btn-lg btn-danger\">" << translate("Close") << "</button>\n"
    "      </div>\n"
    // "    </div>\n"
    "  </form>\n"
    "</div>\n";
  if (!track.segments.empty()) {
    os <<
      "<div id=\"itinerary-track-map\"></div>\n";
    json features;
    features["tracks"] = {track_id};
    features["routes"] = json::array();
    features["waypoints"] = json::array();
    json segments;
    for (const auto segment : track.segments) {
      if (segment.id.first)
        segments.push_back(segment.id.second);
    }
    features["segments"] = segments;
    json j{
      {"itinerary_id", itinerary_id},
      {"track_id", track_id},
      {"segments", segments}
    };
    // std::cout << j.dump(4) << '\n';
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

void ItineraryTrackEditHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  itinerary_id = std::stol(request.get_param("itineraryId"));
  track_id = std::stol(request.get_param("trackId"));
  const std::string shared = request.get_param("shared");
  read_only = shared == "true";
  const std::string select_all_str = request.get_param("select-all");
  select_all = select_all_str == "on";
  // Page title of the itinerary track editing page
  set_page_title(translate("Itinerary Track Edit"));
  set_menu_item(unknown);
  if (request.method == HTTPMethod::post) {
    action = request.get_param("action");
    if (!action.empty()) {
      selected_segment_id_map = request.extract_array_param_map("segment");
      for (const auto &m : selected_segment_id_map) {
        if (!m.second.empty())
          selected_segment_ids.push_back(m.first);
      }
    }
  }
}

void ItineraryTrackEditHandler::delete_segments(
    const HTTPServerRequest& request,
    ItineraryPgDao &dao)
{
  auto track = dao.get_track(get_user_id(), itinerary_id, track_id);
  bool dirty = false;
  track.segments.erase(
      std::remove_if(
          track.segments.begin(),
          track.segments.end(),
          [&](const ItineraryPgDao::track_segment &segment) {
            bool retval = segment.id.first &&
              selected_segment_id_map.find(segment.id.second) !=
              selected_segment_id_map.end();
            if (retval)
              dirty = true;
            return retval;
          }),
      track.segments.end());
  if (dirty) {
    track.calculate_statistics();
    dao.save(
        get_user_id(),
        itinerary_id,
        track);
  }
}

void ItineraryTrackEditHandler::split_track(
    const HTTPServerRequest& request,
    ItineraryPgDao &dao)
{
  long split_before_id = selected_segment_ids.front();
  auto track = dao.get_track(get_user_id(), itinerary_id, track_id);
  ItineraryPgDao::track new_track;
  std::ostringstream os;
  if (track.name.first) {
    // Name given to new section of a split track.  The parameter is the name of
    // the original track.
    os << format(translate("{1} (split)"))
      % track.name.second;
  } else {
    // Name given to new section of a split track which has no name.  The
    // parameter is the ID of the original track
    os << format(translate("ID: {1,number=left} (split)"))
      % track.id.second;
  }
  new_track.name = std::make_pair(true, os.str());
  new_track.color_key = track.color_key;
  for (auto ts : track.segments) {
    if (ts.id.first && ts.id.second >= split_before_id)
      new_track.segments.push_back(ts);
  }
  // Remove the same segments from original track
  track.segments.erase(
      std::remove_if(
          track.segments.begin(),
          track.segments.end(),
          [split_before_id](const ItineraryPgDao::track_segment &segment) {
            return segment.id.first && segment.id.second >= split_before_id;
          }),
      track.segments.end());
  new_track.calculate_statistics();
  dao.create_track(get_user_id(), itinerary_id, new_track);
  track.calculate_statistics();
  dao.save(
      get_user_id(),
      itinerary_id,
      track);
}

void ItineraryTrackEditHandler::merge_segments(
    const HTTPServerRequest& request,
    ItineraryPgDao &dao)
{
  bool complete = false;
  bool contiguous = true;
  auto track = dao.get_track(get_user_id(), itinerary_id, track_id);
  ItineraryPgDao::track_segment* target = nullptr;
  for (auto i = track.segments.begin(); i != track.segments.end(); i++) {
    if (!i->id.first)
      throw std::invalid_argument("Segment has no ID");
    if (selected_segment_id_map.find(i->id.second) != selected_segment_id_map.end()) {
      if (target) {
        // Have we already found the end of one section
        if (complete) {
          contiguous = false;
        } else {
          // std::cout << "Adding segment id " << i->id.second << " to segment " << target->id.second << '\n';
          for (const auto &p : i->points)
            target->points.push_back(p);
          i->points.clear();
        }
      } else {
        target = &(*i);
        // std::cout << "Target segment (first checked) is " << target->id.second << '\n';
      }
    } else {
      if (target) {
        complete = true;
      // } else {
      //   std::cout << "Segment " << i->id.second << " is not selected\n";
      }
    }
  }
  // if (!contiguous)
  //   std::cerr << "Warning.  The selection is non-contiguous\n";
  if (target) {
    // Remove all empty segments
    track.segments.erase(
        std::remove_if(
            track.segments.begin(),
            track.segments.end(),
            [](const ItineraryPgDao::track_segment &segment) {
              return segment.points.empty();
            }),
        track.segments.end());
    track.calculate_statistics();
    dao.save(get_user_id(),
             itinerary_id,
             track);
  // } else {
  //   std::cout << "No segments merged\n";
  }
}

void ItineraryTrackEditHandler::append_pre_body_end(std::ostream& os) const
{
  BaseMapHandler::append_pre_body_end(os);
  os << "    <script type=\"module\" src=\"" << get_uri_prefix() << "/static/js/itinerary-track-edit.js\"></script>\n";
}

void ItineraryTrackEditHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  ItineraryPgDao dao;
  if (action == "delete") {
    delete_segments(request, dao);
  } else if (action == "split" && !selected_segment_ids.empty()) {
    split_track(request, dao);
    std::ostringstream url;
    url << get_uri_prefix() << "/itinerary?id=" << itinerary_id
        << "&active-tab=features";
    redirect(request, response, url.str());
    return;
  } else if (action == "merge" && selected_segment_ids.size() > 1) {
    merge_segments(request, dao);
  }
  std::map<std::string, std::string> page_param_map;
  page_param_map["itineraryId"] = std::to_string(itinerary_id);
  page_param_map["trackId"] = std::to_string(track_id);
  page_param_map["shared"] = read_only ? "true" : "false";
  page_param_map["select-all"] = select_all ? "on" : "";
  const long total_count = dao.get_track_segment_count(get_user_id(),
                                                       itinerary_id,
                                                       track_id);
  Pagination pagination(get_uri_prefix() + "/itinerary/track/edit",
                        page_param_map,
                        total_count,
                        10,
                        5,
                        true,
                        true,
                        "track-page");
  const std::string page = request.get_param("track-page");
  try {
    if (!page.empty())
      pagination.set_current_page(std::stoul(page));
  } catch (const std::logic_error& e) {
    std::cerr << "Error converting string to page number\n";
  }
  const auto track = dao.get_track_segments(get_user_id(),
                                            itinerary_id,
                                            track_id,
                                            pagination.get_offset(),
                                            pagination.get_limit());
  build_form(response.content, pagination, track);
}
