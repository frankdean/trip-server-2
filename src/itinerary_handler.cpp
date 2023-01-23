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
#include "itinerary_handler.hpp"
#include "itineraries_handler.hpp"
#include "session_pg_dao.hpp"
#include "geo_utils.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>
#include <vector>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::json;

/// The maximum number of location tracking points that can be pasted into an
/// itinerary
const long ItineraryHandler::max_track_points = 1000;

namespace fdsd {
  namespace trip {
    void to_json(nlohmann::json& j, const ItineraryHandler::paste_features& ids)
    {
      ItineraryHandler::paste_features::to_json(j, ids);
    }

    void from_json(const nlohmann::json& j, ItineraryHandler::paste_features& ids)
    {
      ItineraryHandler::paste_features::from_json(j, ids);
    }
  } // namespace trip
} // namespace fdsd

void ItineraryHandler::paste_features::to_json(nlohmann::json& j,
                                               const ItineraryHandler::paste_features& ids)
{
  ItineraryPgDao::selected_feature_ids::to_json(j, ids);
  j["itinerary_id"] = ids.itinerary_id;
}

void ItineraryHandler::paste_features::from_json(
    const nlohmann::json& j,
    ItineraryHandler::paste_features& ids)
{
  ItineraryPgDao::selected_feature_ids::from_json(j, ids);
  j.at("itinerary_id").get_to(ids.itinerary_id);
}

ItineraryPgDao::selected_feature_ids
    ItineraryHandler::get_selected_feature_ids(
        const web::HTTPServerRequest& request)
{
  std::map<long, std::string> route_map =
    request.extract_array_param_map("route");
  std::map<long, std::string> waypoint_map =
    request.extract_array_param_map("waypoint");
  std::map<long, std::string> track_map =
    request.extract_array_param_map("track");
  ItineraryPgDao::selected_feature_ids features;
  for (const auto &m : route_map) {
    if (m.second == "on")
      features.routes.push_back(m.first);
  }
  for (const auto &m : waypoint_map) {
    if (m.second == "on")
      features.waypoints.push_back(m.first);
  }
  std::vector<long> track_ids;
  for (const auto &m : track_map) {
    if (m.second == "on")
      features.tracks.push_back(m.first);
  }
  return features;
}

void ItineraryHandler::append_heading_content(
    web::HTTPServerResponse& response,
    const ItineraryPgDao::itinerary& itinerary)
{
  response.content
    <<
    "            <div class=\"pt-3\">\n"
    "              <h1>" << x(itinerary.title) << "</h1>\n";
  if (itinerary.shared_to_nickname.first) {
    response.content
      <<
      "            <p><span>Itinerary owner</span>: " << x(itinerary.owner_nickname.second) << "</p>\n";
  }
  if (itinerary.start.first || itinerary.finish.first) {
    response.content
      <<
      "              <div>\n"
      "                <hr>\n"
      "                <p>";
    if (itinerary.start.first && !itinerary.finish.first) {
      response.content
        // Shows when an itineray with no end date starts
        << format(translate("From: {1,ftime='%a'} {1,date=medium}"))
        % std::chrono::duration_cast<std::chrono::seconds>(
            itinerary.start.second.time_since_epoch()).count();
    } else if (!itinerary.start.first && itinerary.finish.first) {
      response.content
        // Shows when an itineray with no start date ends
        << format(translate("Until: {1,ftime='%a'} {1,date=medium}"))
        % std::chrono::duration_cast<std::chrono::seconds>(
            itinerary.finish.second.time_since_epoch()).count();
    } else {
      // Shows when an itinerary starts and ends
      response.content
        << format(translate("Between: {1,ftime='%a'} {1,date=medium} "
                            "and {2,ftime='%a'} {2,date=medium}"))
        % std::chrono::duration_cast<std::chrono::seconds>(
            itinerary.start.second.time_since_epoch()).count()
        % std::chrono::duration_cast<std::chrono::seconds>(
            itinerary.finish.second.time_since_epoch()).count();
    }
    response.content
      << as::posix << "</p>\n"
      "              </div>\n";
  }
  response.content << "            </div>\n";
}

void ItineraryHandler::append_itinerary_content(
    web::HTTPServerResponse& response,
    const ItineraryPgDao::itinerary& itinerary)
{
  response.content
    <<
    "          <div id=\"itinerary-tab-content\">\n";
  append_heading_content(response, itinerary);
  if (itinerary.description.first) {
    response.content
      <<
      "            <hr>\n"
      "            <div id=\"div-view-raw\">\n"
      "              <textarea id=\"raw-textarea\" class=\"raw-markdown\" rows=\"12\"";
    if (read_only)
      response.content << " readonly";
    response.content
      <<
      ">\n"
      << x(itinerary.description.second) <<
      "              </textarea>\n"
      "            </div>\n";
  }
  response.content
    <<
    "          </div> <!-- #itinerary-tab-content -->\n";
}

/**
 * Appends the past path, either a route or a track, to the specified output
 * stream.
 *
 * \param os output stream
 * \param path list of path points
 * \param path_type either 'route' or 'track' used to name checkbox elements
 * \param estimate_time whether to show an estimate of time to hike the path on
 * foot, based on Scarf's Equivalance.
 */
void ItineraryHandler::append_path(
    std::ostream &os,
    const ItineraryPgDao::path_summary &path,
    std::string path_type,
    bool estimate_time)
{
  os
    <<
    "                        <tr>\n"
    "                          <td>\n"
    "                            <input id=\"input-" << path_type << "-" << path.id.second << "\" type=\"checkbox\" name=\"" << path_type << "[" << path.id.second << "]\">\n"
    "                            <label for=\"input-" << path_type << "-" << path.id.second << "\">";
  if (path.name.first) {
    os << x(path.name.second);
  } else {
    // Database ID or an item, typically a route, track or waypoint
    os << format(translate("ID:&nbsp;{1,number=left}")) % path.id.second;
  }
  os
    <<
    "</label>\n"
    "                          </td>\n"
    "                          <td>" << (path.color.first ? x(path.color.second) : "") << "</td>\n"
    "                          <td>";
  if (path.distance.first) {
    os <<
      // Shows distance in kilometers
      format(translate("{1,num=fixed,precision=2}&nbsp;km")) % path.distance.second;
  }
  os
    <<
    "<td>\n"
    "                          <td>";
  if (path.distance.first) {
    os <<
      // Shows distance in miles
      format(translate("{1,num=fixed,precision=2}&nbsp;mi")) % (path.distance.second / kms_per_mile);
  }
  os << "<td>\n";
  if (estimate_time) {
    const double distance =  path.distance.first ? path.distance.second : 0;
    const double ascent = path.ascent.first ? path.ascent.second : 0;
    double estimated_hiking_time =
      calculate_scarfs_equivalence_kilometers(distance, ascent);
    os << "                          <td>";
    if (std::round(estimated_hiking_time * 100) > 0) {
      const double hours = std::floor(estimated_hiking_time);
      const double minutes = (estimated_hiking_time - hours) * 60;
      os
        <<
        // abbreviation for showing a period of time in hours and minutes
        format(translate("{1,num=fixed,precision=0}&nbsp;hrs {2,num=fixed,precision=0}&nbsp;mins")) % hours % minutes;
    }
    os
      << "</td>\n";
  }
  if (path.ascent.first || path.descent.first) {
    const double ascent = path.ascent.first ? path.ascent.second : 0;
    const double descent = path.descent.first ? path.descent.second : 0;
    os
      <<
      "                          <td>"
      // Shows the total ascent and descent of a path in meters
      << format(translate("↗{1,num=fixed,precision=0}&nbsp;m ↘{2,num=fixed,precision=0}&nbsp;m")) % ascent % descent
      << "</td>\n"
      "                          <td>"
      // Shows the total ascent and descent of a path in ft
      << format(translate("↗{1,num=fixed,precision=0}&nbsp;ft ↘{2,num=fixed,precision=0}&nbsp;ft"))
      % (ascent / feet_per_meter) % (descent / feet_per_meter)
      << "</td>\n";
  } else {
    os << "                          <td></td><td></td>\n";
  }
  if (path.highest.first || path.lowest.first) {
    const double highest = path.highest.first ? path.highest.second : 0;
    const double lowest = path.lowest.first ? path.lowest.second : 0;
    os
      <<
      "                          <td>"
      // Shows the total highest and lowest points of a path in meters
      << format(translate("{1,num=fixed,precision=0}&#x21c5;{2,num=fixed,precision=0}&nbsp;m")) % highest % lowest
      << "</td>\n"
      "                          <td>"
      // Shows the total highest and lowest points of a path in ft
      << format(translate("{1,num=fixed,precision=0}&#x21c5;{2,num=fixed,precision=0}&nbsp;ft"))
      % (highest / feet_per_meter) % (lowest / feet_per_meter)
      << "</td>\n";
  } else {
    os << "                          <td></td><td></td>\n";
  }

  os
    <<
    "                        </tr>\n";
}

void ItineraryHandler::append_waypoint(
    std::ostream &os,
    const ItineraryPgDao::waypoint_summary &waypoint)
{
  os
    <<
    "                      <tr>\n"
    "                        <td>\n"
    "                          <input id=\"input-waypoint-" << waypoint.id << "\" type=\"checkbox\" name=\"waypoint[" << waypoint.id << "]\">\n"
    "                          <label for=\"input-waypoint-" << waypoint.id << "\">";
  if (waypoint.name.first) {
    os << x(waypoint.name.second);
  } else {
    // Database ID or an item, typically a route, track or waypoint
    os << format(translate("ID:&nbsp;{1,number=left}")) % waypoint.id;
  }
  os <<
    "</label>\n"
    "                        </td>\n"
    "                        <td>";
  if (waypoint.symbol.first) {
    os << x(waypoint.symbol.second);
  }
  os <<
    "</td>\n"
    "                        <td>";
  if (waypoint.comment.first) {
    os << x(waypoint.comment.second);
  }
  os <<
    "</td>\n"
    "                        <td>";
  if (waypoint.type.first) {
    os << x(waypoint.type.second);
  }
  os <<
    "</td>\n"
    "                      </tr>\n";
}

void ItineraryHandler::append_features_content(
    web::HTTPServerResponse& response,
    const ItineraryPgDao::itinerary& itinerary)
{
  response.content
    <<
    "          <div id=\"features-tab-content\">\n";
  append_heading_content(response, itinerary);
  if (itinerary.routes.empty()
      && itinerary.tracks.empty()
      && itinerary.waypoints.empty()) {

    response.content
      <<
      "            <div class=\"alert alert-info\" role=\"alert\">\n"
      // Message displayed when there are no routes, tracks or waypoints to display on the Itinerary page
      "              <p>" << translate("There are no features to display") << "</p>\n"
      "            </div>\n";
  } else {
    response.content
      <<
      "            <input id=\"input-select-all\" type=\"checkbox\" accesskey=\"a\" onclick=\"select_all(this, 'features-tab-content')\">\n"
      // Checkbox label shown to select all items on a page
      "            <label for=\"input-select-all\" class=\"text-danger\">" << translate("Select all") << "</label>\n"
      "            <div class=\"accordion\" id=\"featuresAccordion\">\n"
      "              <div class=\"accordion-item\">\n"
      "                <h4 class=\"accordion-header\" id=\"routesHeading\">\n"
      "                  <button class=\"accordion-button\" type=\"button\" data-bs-toggle=\"collapse\" data-bs-target=\"#collapseRoutes\" aria-expanded=\"true\" aria-controls=\"collapseRoutes\">\n"
      // Title of the Routes section on the Itinerary page showing the count of routes
      "                    " << format(translate("Routes ({1,num})")) % itinerary.routes.size() << "\n"
      "                  </button>\n"
      "                </h4>\n"
      "                <div id=\"collapseRoutes\" class=\"accordion-collapse collapse show\" aria-labelledby=\"routesHeading\">\n"
      "                  <div class=\"accordion-body\">\n";
    if (!itinerary.routes.empty()) {
      response.content
        <<
        "                    <div class=\"table-responsive\">\n"
        "                      <table class=\"table table-striped\">\n"
        "                        <tr>\n"
        "                          <td colspan=\"11\">\n"
        "                            <input id=\"input-select-all-routes\" type=\"checkbox\" accesskey=\"r\" onclick=\"select_all(this, 'collapseRoutes')\">\n"
        // Checkbox label shown to select all routes on the itinerary page
        "                            <label for=\"input-select-all-routes\" class=\"text-danger\">" << translate("Select all routes") << "</label>\n"
        "                          </td>\n"
        "                        </tr>\n";
      for (const auto &route : itinerary.routes) {
        append_path(response.content, route, "route", true);
      }
      response.content
        <<
        "                      </table>\n"
        "                    </div>\n";
    }
    response.content
      <<
      "                  </div>\n"
      "                </div>\n"
      "              </div>\n"
      "              <div class=\"accordion-item\">\n"
      "                <h4 class=\"accordion-header\" id=\"waypointsHeading\">\n"
      "                  <button class=\"accordion-button\" type=\"button\" data-bs-toggle=\"collapse\" data-bs-target=\"#collapseWaypoints\" aria-expanded=\"true\" aria-controls=\"collapseWaypoints\">\n"
      // Title of the Waypoints section on the Itinerary page showing the count of waypoints
      "                    " << format(translate("Waypoints ({1,num})")) % itinerary.waypoints.size() << "\n"
      "                  </button>\n"
      "                </h4>\n"
      "                <div id=\"collapseWaypoints\" class=\"accordion-collapse collapse show\" aria-labelledby=\"waypointsHeading\">\n"
      "                  <div class=\"accordion-body\">\n";
    if (!itinerary.waypoints.empty()) {
      response.content
        <<
        "                  <div class=\"table-responsive\">\n"
        "                    <table class=\"table table-striped\">\n"
        "                      <tr>\n"
        "                        <td colspan=\"4\">\n"
        "                         <input id=\"input-select-all-waypoints\" type=\"checkbox\" accesskey=\"w\" onclick=\"select_all(this, 'collapseWaypoints')\">\n"
        // Checkbox label shown to select all waypoints on the itinerary page
        "                         <label for=\"input-select-all-waypoints\" class=\"text-danger\">" << translate("Select all waypoints") << "</label>\n"
        "                        </td>\n"
        "                      </tr>\n";
      for (const auto &waypoint : itinerary.waypoints) {
        append_waypoint(response.content, waypoint);
      }
      response.content
        <<
        "                      </table>\n"
        "                    </div>\n";
    }
    response.content
      <<
      "                  </div>\n"
      "                </div>\n"
      "              </div>\n"
      "              <div class=\"accordion-item\">\n"
      "                <h4 class=\"accordion-header\" id=\"tracksHeading\">\n"
      "                  <button class=\"accordion-button\" type=\"button\" data-bs-toggle=\"collapse\" data-bs-target=\"#collapseTracks\" aria-expanded=\"true\" aria-controls=\"collapseTracks\">\n"
      // Title of the Tracks section on the Itinerary page showing the count of tracks
      "                    " << format(translate("Tracks ({1,num})")) % itinerary.tracks.size() << "\n"
      "                  </button>\n"
      "                </h4>\n"
      "                <div id=\"collapseTracks\" class=\"accordion-collapse collapse show\" aria-labelledby=\"tracksHeading\">\n"
      "                  <div class=\"accordion-body\">\n";
    if (!itinerary.tracks.empty()) {
      response.content
        <<
        "                    <div class=\"table-responsive\">\n"
        "                      <table class=\"table table-striped\">\n"
        "                        <tr>\n"
        "                          <td colspan=\"10\">\n"
        "                            <input id=\"input-select-all-tracks\" type=\"checkbox\" accesskey=\"t\" onclick=\"select_all(this, 'collapseTracks')\">\n"
        // Checkbox label shown to select all tracks on the itinerary page
        "                            <label for=\"input-select-all-tracks\" class=\"text-danger\">" << translate("Select all tracks") << "</label>\n"
        "                          </td>\n"
        "                        </tr>\n";
      for (const auto &track : itinerary.tracks) {
        append_path(response.content, track, "track", false);
      }
      response.content
        <<
        "                      </table>\n"
        "                    </div>\n";
    }
    response.content
      <<
      "                  </div>\n"
      "                </div>\n"
      "              </div>\n"
      "            </div>\n";
  } // no features to display
  response.content << "          </div> <!-- #features-tab-content -->\n";
}

void ItineraryHandler::build_form(web::HTTPServerResponse& response,
                                  const ItineraryPgDao::itinerary& itinerary)
{
  std::string itinerary_active = (active_tab == itinerary_tab ? " active" : "");
  std::string itinerary_aria_selected = (active_tab == itinerary_tab ? "true" : "false");
  std::string features_active = (active_tab == features_tab ? " active" : "");
  std::string features_aria_selected = (active_tab == features_tab ? "true" : "false");
  if (max_track_paste_exceeded) {
    response.content
      <<
      "            <div class=\"alert alert-warning\" role=\"alert\">\n"
      // Message displayed when attempting to paste more than the maximum number of items into an itinerary
      "              <p>" << format(translate("Pasted the maximum of {1} points")) % max_track_points << "</p>\n"
      "            </div>\n";
  }
  response.content
    <<
    "  <form method=\"post\">\n"
    "    <input type=\"hidden\" name=\"id\" value=\"" << itinerary_id << "\" >\n"
    "    <div class=\"container-fluid\">\n"
    "      <ul class=\"nav nav-tabs\" id=\"myTab\" role=\"tablist\">\n"
    "        <li class=\"nav-item\" role=\"presentation\">\n"
    // Label of the tab on the itinerary page that shows general information about the itinerary
    "          <button class=\"nav-link" << itinerary_active << "\" id=\"itinerary-tab\" data-bs-toggle=\"tab\" data-bs-target=\"#itinerary-tab-pane\" type=\"button\" accesskey=\"i\" role=\"tab\" aria-controls=\"itinerary-tab-pane\" aria-selected=\"" << itinerary_aria_selected << "\">" << translate("Itinerary") << "</button>\n"
    "        </li>\n"
    "        <li class=\"nav-item\" role=\"presentation\">\n"
    // Label of the tab on the itinerary page that shows routes, tracks and waypoints belonging to the itinerary
    "          <button class=\"nav-link" << features_active << "\" id=\"features-tab\" data-bs-toggle=\"tab\" data-bs-target=\"#features-tab-pane\" type=\"button\" accesskey=\"f\" role=\"tab\" aria-controls=\"features-tab-pane\" aria-selected=\"" << features_aria_selected << "\">" << translate("Features") << "</button>\n"
    "        </li>\n"
    "      </ul>\n"
    "      <div class=\"tab-content\" id=\"myTabContent\">\n"
    "        <!-- Content for itinerary tab -->\n"
    "        <div class=\"tab-pane fade show" << itinerary_active << "\" id=\"itinerary-tab-pane\" role=\"tabpanel\" aria-labelledby=\"itinerary-tab\" tabindex=\"0\">\n"
    "          <ul class=\"nav nav-pills\">\n";
  if (!read_only) {
    response.content
      <<
      "            <li class=\"nav-item\">\n"
      // Label for a menu item on the general information tab of the itinerary
      // page that allows the general information to be viewed
      "              <button accesskey=\"e\" class=\"nav-link\" formmethod=\"post\" formaction=\"" << get_uri_prefix() << "/itinerary/edit\">" << translate("Edit") << "</button> <!-- writable version -->\n"
      "            </li>\n"
      "            <li class=\"nav-item\">\n"
      // Label for a menu item on the general information tab of the itinerary page to show the itinerary sharing page
      "              <a class=\"nav-link\" href=\"" << get_uri_prefix() << "/itinerary-sharing?id=" << itinerary_id << "\">" << translate("Sharing") << "</a> <!-- writable version -->\n"
      "            </li>\n";
  } else {
    response.content
      <<
      "            <li class=\"nav-item d-none\">\n"
      // Label for a menu item on the general information tab of the itinerary page to show the raw Markdown text
      "              <a class=\"nav-link\">" << translate("View raw Markdown") << "</a> <!-- read-only version -->\n"
      "            </li>\n"
      "            <li class=\"nav-item opacity-50\">\n"
      // Label for a menu item on the general information tab of the itinerary page to hide the raw Markdown text
      "              <a class=\"nav-link\">" << translate("Hide raw Markdown") << "</a> <!-- read-only version -->\n"
      "            </li>\n";
  }
  response.content
    <<
    "            <li class=\"nav-item\">\n"
    // Label for a link to the Markdown syntax help page
    "              <a class=\"nav-link\" href=\"http://daringfireball.net/projects/markdown/syntax\">" << translate("What is Markdown?") << "</a> <!-- read-only version -->\n"
    "            </li>\n"
    "            <li class=\"nav-item\">\n"
    // Label for a menu item that closes a page
    "              <a accesskey=\"c\" class=\"nav-link\" href=\"" << get_uri_prefix() << "/itineraries\">" << translate("Close") << "</a>\n"
    "            </li>\n"
    "          </ul>\n"
    "          <!-- body of itinerary tab -->\n";
  append_itinerary_content(response, itinerary);
  response.content
    <<
    "        </div> <!-- #itinerary-tab-pane -->\n"
    "        <!-- Content for features tab -->\n"
    "        <div class=\"tab-pane fade show" << features_active << "\" id=\"features-tab-pane\" role=\"tabpanel\" aria-labelledby=\"features-tab\" tabindex=\"0\">\n"
    "          <ul class=\"nav nav-pills\">\n"
    "            <li class=\"nav-item dropdown\">\n"
    // Label for drop-down menu item containing 'view' type menu links
    "              <a class=\"nav-link dropdown-toggle\" data-bs-toggle=\"dropdown\" href=\"#\" role=\"button\" aria-expanded=\"false\">" << translate("View") << "</a>\n"
    "              <ul class=\"dropdown-menu\">\n";
  if (read_only) {
    response.content
      <<
      // Label for menu item to view waypoint details
      "                <li><button class=\"dropdown-item\" name=\"action\" value=\"attributes\" formmethod=\"post\">" << translate("Waypoint") << "</button></li> <!-- read-only version -->\n"
      // Label for menu item to view details of a path, which is either a selected route or track
      "                <li><a class=\"dropdown-item opacity-50\">" << translate("Path") << "</a></li> <!-- read-only version -->\n";
  }
  response.content
    <<
    // Label for menu item to re-fetch the page with updated data
    "                <li><button class=\"dropdown-item\" name=\"action\" value=\"refresh\">" << translate("Refresh") << "</button></li>\n"
    "                <li><hr class=\"dropdown-divider\"></li>\n"
    // Label for menu item to display the map page showing the selected routes, tracks and waypoints
    "                <li><button class=\"dropdown-item\" accesskey=\"m\" formmethod=\"post\" formaction=\"" << get_uri_prefix() << "/itinerary-map?id=" << itinerary_id << "\">" << translate("Show map") << "</button></li>\n"
    "              </ul>\n"
    "            </li>\n"
    "            <li class=\"nav-item dropdown\">\n"
    // Label for drop-down menu item containing 'edit' type menu links
    "              <a class=\"nav-link dropdown-toggle\" data-bs-toggle=\"dropdown\" href=\"#\" role=\"button\" aria-expanded=\"false\">" << translate("Edit") << "</a>\n"
    "              <ul class=\"dropdown-menu\">\n";
  if (!read_only) {
    response.content
      <<
      // Label for menu item to create a new itinerary waypoint
      "                <li><a class=\"dropdown-item\" href=\"" << get_uri_prefix() << "/itinerary-wpt-edit?itineraryId=" << itinerary_id << "\">" << translate("Add waypoint") << "</a></li> <!-- writable version -->\n"
      "                <li><hr class=\"dropdown-divider\"></li> <!-- writable version -->\n"
      // Label for menu item to edit some or all of the details of a single selected route, track or waypoint
      "                <li><button class=\"dropdown-item\" formmethod=\"post\" name=\"action\" value=\"attributes\">" << translate("Attributes") << "</button></li> <!-- writable version -->\n"
      // Label for menu item to edit the segments and points belonging to a route or track
      "                <li><a class=\"dropdown-item opacity-50\">" << translate("Path") << "</a></li> <!-- writable version -->\n"
      "                <li><hr class=\"dropdown-divider\"></li> <!-- writable version -->\n"
      // Label for menu item to join together selected routes or tracks
      "                <li><a class=\"dropdown-item opacity-50\">" << translate("Join paths") << "</a></li> <!-- writable version -->\n"
      "                <li><hr class=\"dropdown-divider\"></li> <!-- writable version -->\n";
  }
  response.content
    <<
    // Label for menu item to copy the selected items, similarly to copy and paste functions
    "                <li><button class=\"dropdown-item\" name=\"action\" value=\"copy\" accesskey=\"y\">" << translate("Copy selected items") << "</button></li>\n"
    "                <li><hr class=\"dropdown-divider\"></li> <!-- writable version -->\n";
  if (!read_only) {
    response.content
      <<
      "                <li><button class=\"dropdown-item\" accesskey=\"l\" formmethod=\"post\" name=\"action\" value=\"auto-color\""
      // Confirmation dialog text when converting one or more tracks to routes or vice-versa
      " onclick=\"return confirm('" << translate("Automatically assign colors to selected tracks and routes?")
      << "');\">"
      // Label for menu item to set a sequence of colors to the selected routes and tracks
      << translate("Assign colors to routes and tracks") << "</button></li> <!-- writable version -->\n";

    if (location_history_paste_params.first ||
        selected_features_paste_params.first) {

      response.content
        <<
        "                <li><button class=\"dropdown-item\" accesskey=\"p\" name=\"action\" value=\"paste\""
        // Confirmation dialog text when pasting features to itinerary
        " onclick=\"return confirm('" << translate("Paste items?")
        << "');\">"
        // Label for menu item to set paste selected items into an itinerary
        << translate("Paste") << "</button></li> <!-- writable version -->\n";
    }

    response.content
      <<
      "                <li><button class=\"dropdown-item\" accesskey=\"v\" formmethod=\"post\" name=\"action\" value=\"convert\" "
      // Confirmation dialog text when converting one or more tracks to routes or vice-versa
      "onclick=\"return confirm('" << translate("Convert selected tracks and routes?") << "');\">"
      // Label for menu item to convert the selected tracks to routes
      << translate("Convert tracks and routes") << "</button></li> <!-- writable version -->\n"
      // Label for menu item to reduce the number of points in a track using a simplification algorithm
      "                <li><button class=\"dropdown-item\" accesskey=\"s\" formmethod=\"post\" formaction=\"" << get_uri_prefix() << "/itinerary/simplify/path?id=" << itinerary_id << "\">" << translate("Simplify track") << "</button></li> <!-- writable version -->\n"
      "                <li><hr class=\"dropdown-divider\"></li> <!-- writable version -->\n"
      // Label for menu item to permanently delete the selected items
      "                <li><button class=\"dropdown-item\" accesskey=\"d\" formmethod=\"post\" name=\"action\" value=\"delete_features\" "
      // Confirmation dialog text when deleting a one or more selected itinerary featues
      "onclick=\"return confirm('" << translate("Delete the selected waypoints, routes and tracks?") << "');\">" << translate("Delete selected items") << "</button></li> <!-- writable version -->\n"
      "                <li><hr class=\"dropdown-divider\"></li>\n";
  }
  response.content
    <<
    // Label for menu item to create a complete copy of the current Itinerary as a new Itinerary
    "                <li><a class=\"dropdown-item opacity-50\">" << translate("Create duplicate itinerary") << "</a></li>\n"
    "              </ul>\n"
    "            </li>\n"
    "            <li class=\"nav-item dropdown\">\n"
    // Label for drop-down menu item containing 'data transfer' type menu links, e.g. download, upload
    "              <a class=\"nav-link dropdown-toggle\" data-bs-toggle=\"dropdown\" href=\"#\" role=\"button\" aria-expanded=\"false\">" << translate("Transfer") << "</a>\n"
    "              <ul class=\"dropdown-menu\">\n";
  if (!read_only) {
    response.content
      <<
      // Label for menu item to upload a GPX (GPS data) file containing routes, tracks and waypoints
      "                <li><button class=\"dropdown-item\" accesskey=\"u\" formmethod=\"get\" formaction=\"" << get_uri_prefix() << "/itinerary/upload?id=" << itinerary_id << "\">" << translate("Upload GPX") << "</button></li> <!-- writable version -->\n"
      "                <li><hr class=\"dropdown-divider\"></li> <!-- writable version -->\n";
  }
  response.content
    <<
    // Label for menu item to download the selected routes, tracks and waypoints as a GPX format (GPS data) file
    "                <li><button class=\"dropdown-item\" formmethod=\"post\" formaction=\"" << get_uri_prefix() << "/itinerary/download\" name=\"action\" accesskey=\"g\" value=\"download-gpx\">" << translate("Download GPX") << "</button></li>\n"
    // Label for menu item to download the selected routes, tracks and waypoints as a KML format (GPS data) file
    "                <li><a class=\"dropdown-item opacity-50\">" << translate("Download KML") << "</a></li>\n"
    "                <li><hr class=\"dropdown-divider\"></li>\n"
    // Label for menu item to download the entire Itinerary in a YAML formatted text file
    "                <li><button class=\"dropdown-item\" accesskey=\"n\" formaction=\"" << get_uri_prefix() << "/itinerary/export" << "\">" << translate("Download full itinerary") << "</button></li>\n"
    "              </ul>\n"
    "            </li>\n"
    "            <li class=\"nav-item\">\n"
    "              <a class=\"nav-link\" href=\"" << get_uri_prefix() << "/itineraries\">Close</a>\n"
    "            </li>\n"
    "          </ul>\n"
    "          <!-- body of features tab -->\n";
  append_features_content(response, itinerary);
  response.content
    <<
    "        </div> <!-- #features-tab-pane -->\n"
    "      </div> <!-- #myTabContent -->\n"
    "      <!-- end of nav tabs -->\n"
    "    </div><!-- container-fluid -->\n"
    "  </form>\n"
    "<script>\n"
    "<!--\n"
    "function select_all(cb, name) {\n"
    "  const div = document.getElementById(name);\n"
    "  const all = div.getElementsByTagName('input');\n"
    "  for (let i = 0; i < all.length; i++) {\n"
    "    if (all[i] !== cb && all[i].type == 'checkbox') {\n"
    "      all[i].checked = cb.checked;\n"
    "    }\n"
    "  }\n"
    "}\n"
    "// -->\n"
    "</script>\n";
}

void ItineraryHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  set_page_title(translate("Itinerary"));
  set_menu_item(itinerary);
  std::string id = request.get_query_param("id");
  if (id.empty()) {
    id = request.get_query_param("itinerary_id");
  }
  itinerary_id = std::stol(id);
  if (request.get_param("active-tab") == "features") {
    active_tab = features_tab;
  }
  location_history_paste_params = get_location_history_paste_params();
  selected_features_paste_params = get_selected_features_paste_params();
}

void ItineraryHandler::convertTracksToRoutes(
    const web::HTTPServerRequest& request)
{
  auto selected_features = get_selected_feature_ids(request);
  ItineraryPgDao::itinerary_features features;
  ItineraryPgDao dao;
  if (!selected_features.tracks.empty()) {
    auto tracks = dao.get_tracks(get_user_id(),
                                 itinerary_id,
                                 selected_features.tracks);
    for (const auto &t : tracks)
      features.routes.push_back(ItineraryPgDao::route(t));
  }
  if (!selected_features.routes.empty()) {
    auto routes = dao.get_routes(get_user_id(),
                                 itinerary_id,
                                 selected_features.routes);
    for (const auto &r : routes)
      features.tracks.push_back(ItineraryPgDao::track(r));
  }
  dao.create_itinerary_features(get_user_id(),
                                itinerary_id,
                                features);
}

void ItineraryHandler::auto_color_paths(
    const web::HTTPServerRequest& request)
{
  auto selected_features = get_selected_feature_ids(request);
  if (selected_features.routes.empty() && selected_features.tracks.empty())
    return;
  ItineraryPgDao dao;
  dao.auto_color_paths(get_user_id(), itinerary_id, selected_features);
}

std::pair<bool, TrackPgDao::location_search_query_params>
    ItineraryHandler::get_location_history_paste_params()
{
  std::pair<bool, TrackPgDao::location_search_query_params> retval;
  SessionPgDao session_dao;
  const std::string p =
    session_dao.get_value(get_session_id(), SessionPgDao::location_history_key);
  if (const bool key_exists = !p.empty()) {
    try {
      json j = json::parse(p);
      retval.second = j.get<TrackPgDao::location_search_query_params>();
      retval.first = key_exists;
    } catch (const std::exception& e) {
      std::cerr << "Error parsing location history parameters from session: "
                << e.what() << '\n';
    }
  }
  return retval;
}

std::pair<bool, ItineraryHandler::paste_features>
    ItineraryHandler::get_selected_features_paste_params()
{
  std::pair<bool, paste_features> retval;
  SessionPgDao session_dao;
  const std::string p =
    session_dao.get_value(get_session_id(), SessionPgDao::itinerary_features_key);
  if (const bool key_exists = !p.empty()) {
    try {
      json j = json::parse(p);
      retval.second = j.get<ItineraryHandler::paste_features>();
      retval.first = key_exists;
    } catch (const std::exception& e) {
      std::cerr << "Error parsing itinerary features parameters from session: "
                << e.what() << '\n';
    }
  }
  return retval;
}

void ItineraryHandler::paste_locations()
{
  TrackPgDao track_dao;
  location_history_paste_params.second.page_offset = 0;
  location_history_paste_params.second.page_size =
    ItineraryHandler:: max_track_points;
  TrackPgDao::tracked_locations_result
    locations_result =
    track_dao.get_tracked_locations(location_history_paste_params.second);
  max_track_paste_exceeded =
    locations_result.total_count > ItineraryHandler::max_track_points;
  std::vector<ItineraryPgDao::waypoint> waypoints;
  std::vector<ItineraryPgDao::track_point> points;
  for (const auto &location : locations_result.locations) {
    ItineraryPgDao::track_point point(location);
    points.push_back(point);
    if (location.note.first && !location.note.second.empty()) {
      ItineraryPgDao::waypoint waypoint(location);
      waypoints.push_back(waypoint);
    }
  }
  std::vector<ItineraryPgDao::track_segment> segments;
  ItineraryPgDao::track_segment segment(points);
  segments.push_back(segment);
  ItineraryPgDao::track track(segments);
  track.calculate_statistics();
  ItineraryPgDao dao;
  dao.create_waypoints(get_user_id(), itinerary_id, waypoints);
  std::vector<ItineraryPgDao::track> tracks;
  tracks.push_back(track);
  dao.create_tracks(get_user_id(), itinerary_id, tracks);
}

void ItineraryHandler::paste_itinerary_features()
{
  ItineraryPgDao dao;
  ItineraryPgDao::itinerary_features features;
  if (!selected_features_paste_params.second.routes.empty()) {
    features.routes = dao.get_routes(
        get_user_id(),
        selected_features_paste_params.second.itinerary_id,
        selected_features_paste_params.second.routes);
    for (auto &route : features.routes)
      route.calculate_statistics();
  }
  if (!selected_features_paste_params.second.waypoints.empty()) {
    features.waypoints = dao.get_waypoints(
        get_user_id(),
        selected_features_paste_params.second.itinerary_id,
        selected_features_paste_params.second.waypoints);
  }
  if (!selected_features_paste_params.second.tracks.empty()) {
    features.tracks = dao.get_tracks(
        get_user_id(),
        selected_features_paste_params.second.itinerary_id,
        selected_features_paste_params.second.tracks);
    for (auto &track : features.tracks)
      track.calculate_statistics();
  }
  dao.create_itinerary_features(get_user_id(),
                                itinerary_id,
                                features);
}

void ItineraryHandler::paste_items()
{
  if (location_history_paste_params.first) {
    paste_locations();
    SessionPgDao dao;
    dao.remove_value(get_session_id(), SessionPgDao::location_history_key);
    location_history_paste_params.first = false;
  }
  if (selected_features_paste_params.first) {
    paste_itinerary_features();
    SessionPgDao dao;
    dao.remove_value(get_session_id(), SessionPgDao::itinerary_features_key);
    selected_features_paste_params.first = false;
  }
}

void ItineraryHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  const std::string action = request.get_param("action");
  // std::cout << "Action: \"" << action << "\"\n";
  // auto pp = request.get_post_params();
  // for (auto const &p : pp) {
  //   std::cout << "param: \"" << p.first << "\" -> \"" << p.second << "\"\n";
  // }
  ItineraryPgDao dao;
  if (action == "delete_features") {
    auto features = get_selected_feature_ids(request);
    dao.delete_features(get_user_id(), itinerary_id, features);
    active_tab = features_tab;
  } else if (action == "refresh") {
    active_tab = features_tab;
  } else if (action == "convert") {
    active_tab = features_tab;
    convertTracksToRoutes(request);
  } else if (action == "auto-color") {
    active_tab = features_tab;
    auto_color_paths(request);
  } else if (action == "copy") {
    active_tab = features_tab;
    auto features = get_selected_feature_ids(request);
    selected_features_paste_params.second =
      paste_features(itinerary_id, features);
    selected_features_paste_params.first =
      !(selected_features_paste_params.second.routes.empty() &&
        selected_features_paste_params.second.waypoints.empty() &&
        selected_features_paste_params.second.tracks.empty());
    if (selected_features_paste_params.first) {
      json j = selected_features_paste_params.second;
      SessionPgDao session_dao;
      session_dao.save_value(get_session_id(),
                             SessionPgDao::itinerary_features_key, j.dump());
    }
  } else if (action == "paste") {
    active_tab = features_tab;
    paste_items();
  } else if (action == "attributes") {
    auto features = get_selected_feature_ids(request);
    if (!features.waypoints.empty()) {
      long waypoint_id = features.waypoints.front();
      std::ostringstream url;
      url << get_uri_prefix() << "/itinerary-wpt-edit?itineraryId=" << itinerary_id
          << "&waypointId=" << waypoint_id;
      redirect(request, response, url.str());
    }
  }
  auto itinerary = dao.get_itinerary_details(get_user_id(), itinerary_id);
  if (!itinerary.first)
    throw BadRequestException("Itinerary ID not found");
  read_only = itinerary.second.shared_to_nickname.first;
  build_form(response, itinerary.second);
}
