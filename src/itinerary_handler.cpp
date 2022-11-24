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
#include "geo_utils.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include <boost/locale.hpp>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

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
    std::shared_ptr<ItineraryPgDao::path_summary> path,
    std::string path_type,
    bool estimate_time)
{
  os
    <<
    "                        <tr>\n"
    "                        <td><input type=\"checkbox\" name=\"" << path_type << "[" << path->id.second << "]\"></td>\n"
    "                          <td>";
  if (path->name.first) {
    os << x(path->name.second);
  } else {
    // Database ID or an item, typically a route, track or waypoint
    os << format(translate("ID:&nbsp;{1,number=left}")) % path->id.second;
  }
  os
    << "</td>\n"
    "                          <td>" << (path->color.first ? x(path->color.second) : "") << "</td>\n"
    "                          <td>";
  if (path->distance.first) {
    os <<
      // Shows distance in kilometers
      format(translate("{1,num=fixed,precision=2}&nbsp;km")) % path->distance.second;
  }
  os
    <<
    "<td>\n"
    "                          <td>";
  if (path->distance.first) {
    os <<
      // Shows distance in miles
      format(translate("{1,num=fixed,precision=2}&nbsp;mi")) % (path->distance.second / kms_per_mile);
  }
  os << "<td>\n";
  if (estimate_time) {
    const double distance =  path->distance.first ? path->distance.second : 0;
    const double ascent = path->ascent.first ? path->ascent.second : 0;
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
  if (path->ascent.first || path->descent.first) {
    const double ascent = path->ascent.first ? path->ascent.second : 0;
    const double descent = path->descent.first ? path->descent.second : 0;
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
  if (path->highest.first || path->lowest.first) {
    const double highest = path->highest.first ? path->highest.second : 0;
    const double lowest = path->lowest.first ? path->lowest.second : 0;
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
    std::shared_ptr<ItineraryPgDao::waypoint_summary> waypoint)
{
  os
    <<
    "                      <tr>\n"
    "                        <td><input type=\"checkbox\" name=\"waypoint[" << waypoint->id << "]\"></td>\n"
    "                        <td>";
  if (waypoint->name.first) {
    os << x(waypoint->name.second);
  } else {
    // Database ID or an item, typically a route, track or waypoint
    os << format(translate("ID:&nbsp;{1,number=left}")) % waypoint->id;
  }
  os <<
    "</td>\n"
    "                        <td>";
  if (waypoint->symbol.first) {
    os << x(waypoint->symbol.second);
  }
  os <<
    "</td>\n"
    "                        <td>";
  if (waypoint->comment.first) {
    os << x(waypoint->comment.second);
  }
  os <<
    "</td>\n"
    "                        <td>";
  if (waypoint->type.first) {
    os << x(waypoint->type.second);
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
      "            <div class=\"alert alert-info\">\n"
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
        "                          <td colspan=\"12\">\n"
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
        "                        <td colspan=\"5\">\n"
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
        "                          <td colspan=\"11\">\n"
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
      "            <li class=\"nav-item opacity-50\">\n"
      // Label for a menu item on the general information tab of the itinerary page to show the itinerary sharing page
      "              <a class=\"nav-link\">" << translate("Sharing") << "</a> <!-- writable version -->\n"
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
      "                <li><a class=\"dropdown-item opacity-50\">" << translate("Waypoint") << "</a></li> <!-- read-only version -->\n"
      // Label for menu item to view details of a path, which is either a selected route or track
      "                <li><a class=\"dropdown-item opacity-50\">" << translate("Path") << "</a></li> <!-- read-only version -->\n";
  }
  response.content
    <<
    // Label for menu item to re-fetch the page with updated data
    "                <li><a class=\"dropdown-item opacity-50\">" << translate("Refresh") << "</a></li>\n"
    "                <li><hr class=\"dropdown-divider\"></li>\n"
    // Label for menu item to display the map page showing the selected routes, tracks and waypoints
    "                <li><a class=\"dropdown-item opacity-50\">" << translate("Show map") << "</a></li>\n"
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
      "                <li><a class=\"dropdown-item  opacity-50\">" << translate("Add waypoint") << "</a></li> <!-- writable version -->\n"
      "                <li><hr class=\"dropdown-divider\"></li> <!-- writable version -->\n"
      // Label for menu item to edit some or all of the details of a selected route, track or waypoint
      "                <li><a class=\"dropdown-item opacity-50\">" << translate("Attributes") << "</a></li> <!-- writable version -->\n"
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
    "                <li><a class=\"dropdown-item opacity-50\">" << translate("Copy selected items") << "</a></li>\n"
    "                <li><hr class=\"dropdown-divider\"></li> <!-- writable version -->\n";
  if (!read_only) {
    response.content
      <<
      // Label for menu item to set a sequence of colors to the selected routes and tracks
      "                <li><a class=\"dropdown-item opacity-50\">" << translate("Assign colors to routes and tracks") << "</a></li> <!-- writable version -->\n"
      // Label for menu item to convert the selected tracks to routes
      "                <li><a class=\"dropdown-item opacity-50\">" << translate("Convert tracks to routes") << "</a></li> <!-- writable version -->\n"
      // Label for menu item to reduce the number of points in a track using a simplification algorithm
      "                <li><a class=\"dropdown-item opacity-50\">" << translate("Simplify track") << "</a></li> <!-- writable version -->\n"
      "                <li><hr class=\"dropdown-divider\"></li> <!-- writable version -->\n"
      // Label for menu item to permanently delete the selected items
      "                <li><button class=\"dropdown-item\" accesskey=\"d\" formmethod=\"post\" name=\"action\" value=\"delete_features\" "
      // Confirmation dialog text when deleting a one or more selected itinerary featues
      "onclick=\"return confirm('Delete the selected waypoints, routes and tracks?');\"\">" << translate("Delete selected items") << "</button></li> <!-- writable version -->\n"
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
      "                <li><button class=\"dropdown-item\" accesskey=\"u\" formmethod=\"post\" formaction=\"" << get_uri_prefix() << "/itinerary/upload\">" << translate("Upload GPX") << "</button></li> <!-- writable version -->\n"
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
    "                <li><a class=\"dropdown-item opacity-50\">" << translate("Download full itinerary") << "</a></li>\n"
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
  itinerary_id = std::stol(request.get_query_param("id"));
  if (request.get_param("active-tab") == "features") {
    active_tab = features_tab;
  }
}

void ItineraryHandler::handle_authenticated_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  const std::string action = request.get_post_param("action");
  // std::cout << "Action: \"" << action << "\"\n";
  // auto pp = request.get_post_params();
  // for (auto const &p : pp) {
  //   std::cout << "param: \"" << p.first << "\" -> \"" << p.second << "\"\n";
  // }
  ItineraryPgDao dao;
  if (action == "delete_features") {
    auto features = ItinerariesHandler::get_selected_feature_ids(request);
    dao.delete_features(get_user_id(), itinerary_id, features);
    active_tab = features_tab;
  }
  auto itinerary = dao.get_itinerary_details(get_user_id(), itinerary_id);
  if (!itinerary.first)
    throw BadRequestException("Itinerary ID not found");
  read_only = itinerary.second.shared_to_nickname.first;
  build_form(response, itinerary.second);
}
