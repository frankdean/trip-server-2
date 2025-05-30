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
#include "trip_css_handler.hpp"
#include "../trip-server-common/src/http_response.hpp"

using namespace fdsd::trip;

const std::string TripCssHandler::trip_css_url = "/trip.css";

void TripCssHandler::append_stylesheet_content(
    const fdsd::web::HTTPServerRequest& request,
    fdsd::web::HTTPServerResponse& response) const
{
  (void)request; // unused
  response.set_header("Last-Modified", "Thu, 11 Aug 2022 13:39:53 GMT");
  response.content <<
    ".visually-hidden {\n"
    "    border: 0;\n"
    "    clip: rect(0 0 0 0);\n"
    "    height: auto;\n"
    "    margin: 0;\n"
    "    overflow: hidden;\n"
    "    padding: 0;\n"
    "    position: absolute;\n"
    "    width: 1px;\n"
    "    white-space: nowrap;\n"
    "}\n"
    ".pagination {\n"
    "    list-style: none;\n"
    "    margin: 0;\n"
    "    padding: 0;\n"
    "    display: flex;\n"
    "}\n"
    ".pagination li {\n"
    "    height: 31px;\n"
    "    font-size: 80%;\n"
    "    line-height: 17px;\n"
    "}\n"
    ".pagination a {\n"
    "    display: block;\n"
    "    padding: .5em 1em;\n"
    "    border: 1px solid #ddd;\n"
    "    margin-left: -1px;\n"
    "    text-decoration: none;\n"
    "}\n"
    ".pagination > .disabled {\n"
    "    color: #777;\n"
    "    cursor: not-allowed;\n"
    "    background-color: #fff;\n"
    "    border-color: #ddd;\n"
    "}\n"
    // ".pagination > .disabled > a, .pagination > .disabled > a:hover {\n"
    ".pagination > .disabled > a {\n"
    "    display: inline-block;\n"
    "    pointer-events: none;\n"
    "    text-decoration: none;\n"
    "}\n"
    ".pagination a[aria-current=\"page\"] {\n"
    "    background-color: #337ab7;\n"
    "    color: #fff;\n"
    "}\n";
  // ".div-view {"
  // "  background: red;"
  // "	height: 100%;"
  // "	min-height: 100%;"
  // "   margin-bottom: 40px;"
  // "   padding-bottom: 40px;"
  // "	position: relative;"
  // "}"
  // "#wrapper { background: green; }"
  // "#header { background: yellow; }"
  // "#footer { background: blue; }";
  // "[ng\\:cloak], [ng-cloak], [data-ng-cloak], [x-ng-cloak], .ng-cloak, .x-ng-cloak {"
  //   "  display: none !important;"
  //   "}"
  //   "html, body {"
  //   "	height: 100%;"
  //   "}"
  //   "body {"
  //   "	padding-top: 0px;"
  //   "	padding-bottom: 0px;"
  //   "}"
  //   "#wrapper {"
  //   "	min-height: 100%;"
  //   "	/* height: 100%;  As soon as this height is set, sticky-footer is broken */"
  //   "	position: relative;"
  //   "}"
  //   "#template {"
  //   "	min-height: 100%;"
  //   "	height: 100%;"
  //   "	position: relative;"
  //   "	padding-bottom: 40px;"
  //   "}"
  //   "#pw-strength-wrapper {"
  //   "    max-width: 400px;"
  //   "}"
  //   "#feedback-warning {"
  //   "    max-width: 400px;"
  //   "}"
  //   ".starter-template {"
  //   "  padding: 40px 15px;"
  //   "  text-align: center;"
  //   "}"
  //   ".theme-dropdown .dropdown-menu {"
  //   "  position: static;"
  //   "  display: block;"
  //   "  margin-bottom: 20px;"
  //   "}"
  //   ".theme-showcase > p > .btn {"
  //   "  margin: 5px 0;"
  //   "}"
  //   // ".theme-showcase .navbar .container {"
  //   // "  width: auto;"
  //   // "}"
  //   ".container {"
  //   "  background: red;"
  //   "  margin-top: 20px;"
  //   "  margin-bottom: 20px;"
  //   "}"
  //   "select#nicknameSelect {"
  //   "  width: 200px;"
  //   "}"
  //   "input {"
  //   "	margin: 5px;"
  //   "}"
  //   "input.days, input.minutes, input.hours {"
  //   "	width: 60px;"
  //   "}"
  //   "#input-active {"
  //   "	margin-left: 10px;"
  //   "}"
  //   ".form-signin {"
  //   "  max-width: 330px;"
  //   "  padding: 15px;"
  //   "  margin: 0 auto;"
  //   "}"
  //   ".form-signin .form-signin-heading,"
  //   ".form-signin .checkbox {"
  //   "  margin-bottom: 10px;"
  //   "}"
  //   ".form-signin .checkbox {"
  //   "  font-weight: normal;"
  //   "}"
  //   ".form-signin .form-control {"
  //   "  position: relative;"
  //   "  height: auto;"
  //   "  -webkit-box-sizing: border-box;"
  //   "     -moz-box-sizing: border-box;"
  //   "          box-sizing: border-box;"
  //   "  padding: 10px;"
  //   "  font-size: 16px;"
  //   "}"
  //   ".form-signin .form-control:focus {"
  //   "  z-index: 2;"
  //   "}"
  //   ".form-signin input[type=\"email\"] {"
  //   "  margin-bottom: -1px;"
  //   "  border-bottom-right-radius: 0;"
  //   "  border-bottom-left-radius: 0;"
  //   "}"
  //   ".form-signin input[type=\"password\"] {"
  //   "  margin-bottom: 10px;"
  //   "  border-top-left-radius: 0;"
  //   "  border-top-right-radius: 0;"
  //   "}"
  //   "#footer {"
  //   "  position: absolute;"
  //   "  bottom: 0;"
  //   "  height: 40px;"
  //   "  width: 100%;"
  //   "  padding-top: 10px;"
  //   "  padding-bottom: 10px;"
  //   "  background-color: #f5f5f5;"
  //   "}"
  //   ".angular-leaflet-map {"
  //   "	margin-top: 5px;"
  //   "	margin-left: 10px;"
  //   "	margin-right: 10px;"
  //   "	min-height: 480px;"
  //   "	height: 100%;"
  //   "}"
  //   ".map-page-div {"
  //   "	padding-left: 10px;"
  //   "	padding-right: 10px;"
  //   "}"
  //   ".map-page-buttons {"
  //   "	padding-top: 20px;"
  //   "	padding-bottom: 20px;"
  //   "}"
  //   ".div-view {"
  //   "	height: 100%;"
  //   "	min-height: 100%;"
  //   // "   margin-bottom: 40px;"
  //   // "   padding-bottom: 40px;"
  //   "	position: relative;"
  //   "}"
  //   ".wide-table {"
  //   "	display: block;"
  //   "	overflow: auto;"
  //   "}"
  //   ".itinerary-route-table {"
  //   "	width: 100%;"
  //   "	max-width: 100%;"
  //   "}"
  //   ".itinerary-waypoint-table {"
  //   "  width: 100%;"
  //   "  max-width: 100%;"
  //   "  }"
  //   "  .itinerary-waypoint-table td {"
  //   "    padding-right: 1em;"
  //   "  }"
  //   "  .itinerary-waypoint-table td {"
  //   "    padding-right: 1em;"
  //   "}"
  //   ".itinerary-track-table {"
  //   "  width: 100%;"
  //   "  max-width: 100%;"
  //   "}"
  //   ".logger-param-table {"
  //   "  width: 100%;"
  //   "  max-width: 100%;"
  //   "  margin-bottom: 15px;"
  //   "  }"
  //   "  .logger-param-table th {"
  //   "    padding-right: 1em;"
  //   "  }"
  //   "  .logger-param-table td {"
  //   "    padding-bottom: 1em;"
  //   "    padding-right: 1em;"
  //   "}"
  //   ".select-all-option {"
  //   "	color: red;"
  //   "}"
  //   ".raw-markdown {"
  //   "	width: 100%;"
  //   "}"
  //   ".view-label {"
  //   "	font-weight: bold;"
  //   "}"
  //   ".column-heading {"
  //   "	font-weight: bold;"
  //   "}"
  //   ".box {"
  //   "    display: flex;"
  //   "}"
  //   ".odd .column {"
  //   "    background-color: #f9f9f9;"
  //   "    border-style: solid;"
  //   "    border-color: #f9f9f9;"
  //   "    border-width: 2px 5px 2px 5px;"
  //   "    min-height: 30px;"
  //   "}"
  //   ".even .column {"
  //   "    /* border: 1px solid #e6e6e6; */"
  //   "    border-style: solid;"
  //   "    border-color: white;"
  //   "    border-width: 2px 5px 2px 5px;"
  //   "    min-height: 30px;"
  //   "}"
  //   ".column-one {"
  //   "    flex: 0 0 130px;"
  //   "}"
  //   ".column-two {"
  //   "    flex: 1 1 300px;"
  //   "}"
  //   ".column-three {"
  //   "    flex: 1 4 200px;"
  //   "}"
  //   ".column-select-all {"
  //   "    padding-left: 5px;"
  //   "    background-color: #f9f9f9;"
  //   "}"
  //   ".column-name {"
  //   "    /* flex-grow flex-shrink flex-basis */"
  //   "    flex: 4 0 200px;"
  //   "}"
  //   ".column-color {"
  //   "    flex: 2 0 90px;"
  //     "}"
  //     ".column-distance {"
  //       "    flex: 2 0 70px;"
  //       "    text-align: right;"
  //       "}"
  //       ".column-duration {"
  //       "    flex: 3 0 110px;"
  //       "    text-align: right;"
  //       "}"
  //       ".column-ascent {"
  //       "    flex: 1 0 75px;"
  //       "    text-align: right;"
  //       "}"
  //       ".column-heights {"
  //       "    flex: 0 0 100px;"
  //       "    text-align: right;"
  //       "}"
  //       ".column-symbol {"
  //       "    flex: 1 0 200px;"
  //       "}"
  //       ".column-comment {"
  //       "    flex: 2 0 300px;"
  //       "}"
  //       ".column-category {"
  //       "    flex: 1 0 100px;"
  //       "}"
  //       "#itinerary-search-result .well {"
  //       "    padding: 3px;"
  //       "    margin: 10px;"
  //       "}"
  //       ".text-link{"
  //       "  color: #337ab7;"
  //       "}"
  //       ".text-link:focus, .text-link:hover {"
  //       "  color: #23527c;"
  //       "  text-decoration: underline;"
  //       "}"
  //       "div#itineraries-div-form-buttons {"
  //       "  margin-bottom: 5px;"
  //       "}"
  //       ".tl-goto-page {"
  //       "  margin-bottom: 20px;"
  //       "}"
  //       "/* Fix for href attributes inside accordion etc. http://angular-ui.github.io/bootstrap/versioned-docs/2.4.0/ */"
  //       ".nav, .pagination, .carousel, .panel-title a { cursor: pointer; }";
}
