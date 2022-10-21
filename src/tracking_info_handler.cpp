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
#include "tracking_info_handler.hpp"
#include "tracking_pg_dao.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include "../trip-server-common/src/uuid.hpp"
#include <locale>
#include <boost/locale.hpp>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

void TrackingInfoHandler::do_preview_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  set_page_title("Tracker URL");
  set_menu_item(tracker_info);
}

void TrackingInfoHandler::build_form(
    std::ostream& os,
    std::string uuid,
    bool new_uuid_flag) const
{
  DateTime now;
  os <<
    "<div class=\"container-fluid mse-5\">\n"
    // The title of the Tracker Info page which describes configuring a client
    // track logging page
    "  <h1 class=\"pt-2\">" << translate("Tracker URL") << "</h1>\n";
  if (new_uuid_flag) {
    os <<
      "  <div>\n"
      "    <div id=\"msg-success\" class=\"alert alert-success\" role=\"alert\">\n"
      // Message displayed after generating a new track logging key which is shown following this message
      "      " << translate("New key generated.") << "{" << uuid << "}\n"
      "      <strong>"
      // Message displayed after generating a new track logging key
       << translate("Previous keys will no longer work.  You must configure your tracker client app(s) to use the new key.")
       << "</strong>\n"
      "    </div>\n"
      "  </div>\n";
  }
  // Message shown at the head of a list of suitable location tracking apps
  os <<
    "  <p>" << translate("Use one of the following online tracking applications to log your journeys:") << "</p>\n"
    "    <ul>\n"
    "      <li><a href=\"https://www.fdsd.co.uk/triplogger/\" target=\"_blank\">TripLogger</a></li>\n"
    "      <li><a href=\"http://code.mendhak.com/gpslogger/\" target=\"_blank\">GPSLogger for Android</a></li>\n"
    "      <li><a href=\"http://osmand.net/\">OsmAnd</a>&mdash;<a href=\"https://osmand.net/docs/user/plugins/trip-recording\" target=\"_blank\">trip recording plug-in</a>\n"
    "      <li><a href=\"https://www.traccar.org/client/\" target=\"_blank\">Traccar Client</a></li>\n"
    "    </ul>\n"
    "\n"
    "  <div>\n"
    // General instructions for configuring the parameter settings of apps
    "    <p>"
            << translate(
                "To construct a custom URL for a tracker client, use the parameter names "
                "listed below to pass appropriate values.  The parameter names must all "
                "be in lower-case.") << "</p>\n"
    "    <table class=\"table logger-param-table\">\n"
    // The column heading for parameter keys and values when configuring a tracking logger
    "      <tr><th>" << translate("Parameter") << "</th>\n"
    "          <th class=\"text-center\">" << translate("Value") << "</th>\n"
    // The column heading for mandatory parameters when configuring a tracking logger
    "          <th>" << translate("Mandatory") << "</th>\n"
    "      </tr>\n"
    "      <tr><td>uuid</td><td>" << uuid << "</td><td>&#x2713;</td></tr>\n"
    // Description of 'lat' parameter in table of parameters
    "      <tr><td>lat</td><td>" << translate("Decimal degrees latitude") << "</td><td>&#x2713;</td></tr>\n"
    // Description of 'lon' parameter in table of parameters
    "      <tr><td>lon</td><td>" << translate("Decimal degrees longitude") << "</td><td>&#x2713;</td></tr>\n"
    "      <tr>\n"
    "        <td>time</td>\n"
    // Description of 'time' parameter in table of parameters.  The example shows an ISO 8601 formatted date
    // e.g. 2022-10-20T18:13:13.124Z
    "        <td>" << format(translate("Time in <a href=\"https://en.wikipedia.org/wiki/ISO_8601\">ISO 8601 format</a> e.g. {1}")) % now .get_time_as_iso8601_gmt() << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>unixtime</td>\n"
    // Description of 'unixtime' parameter in table of parameters
    "        <td>" << format(translate("<a href=\"https://en.wikipedia.org/wiki/Unix_time\">Unix time</a> e.g.  {1}")) % now.time_t() << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>mstime</td>\n"
    // Description of 'mstime' parameter in table of parameters
    "        <td>" << format(translate("Unix time in <a href=\"https://en.wikipedia.org/wiki/Millisecond\">milliseconds</a> e.g. {1}")) % now.get_ms() << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>offset</td>\n"
    "        <td>\n"
    // Description of 'offset' parameter in table of parameters
            << translate("Offset to apply to the time value being passed in seconds. e.g. 3600 "
                         "to add one hour.  This is a workaround to situations where it is "
                         "known that the time value is consistently incorrectly reported. "
                         "e.g. A bug causing the GPS time to be one hour slow.  Can be a comma "
                         "separated list of the same length as the 'offsetprovs' parameter.")
            <<
    "        </td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>offsetprovs</td>\n"
    "        <td>\n"
    // Description of 'offsetprovs' parameter in table of parameters
            << translate("Used in conjunction with the 'prov' parameter to apply offsets per "
                         "location provider.  E.g. setting 'offset' to '3600' and 'offsetprovs' to "
                         "'gps' will only apply the offset to locations submitted with the "
                         "prov parameter matching 'gps'.  To apply offset to more than one "
                         "provider, use comma separated lists of the same length.  E.g. set "
                         "'offset' to '3600,7200' and 'offsetprovs' to 'gps,network' to add 1 hour "
                         "to gps times and 2 hours to network times.")
            <<
    "        </td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>msoffset</td>\n"
    // Description of 'msoffset' parameter in table of parameters
    "        <td>\n"
            << translate("Same as the offset parameter above, but in milliseconds.  e.g. 1000 to add one second.") <<
    "        </td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>hdop</td>\n"
    // Description of 'hdop' parameter in table of parameters
    "        <td>" << translate("<a href=\"https://en.wikipedia.org/wiki/Dilution_of_precision_(navigation)\">Horizontal Dilution of Precision</a> (accuracy)") << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>altitude</td>\n"
    // Description of 'altitude' parameter in table of parameters
    "        <td>" << translate("Altitude") << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>speed</td>\n"
    // Description of 'speed' parameter in table of parameters
    "        <td>" << translate("Speed") << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>bearing</td>\n"
    // Description of 'bearing' parameter in table of parameters
    "        <td>" <<translate("Bearing in decimal degrees") << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>sat</td>\n"
    // Description of 'sat' parameter in table of parameters
    "        <td>" << translate("Numeric count of satellites with fixes") << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>prov</td>\n"
    // Description of 'prov' parameter in table of parameters
    "        <td>" << translate("Type of location provider as text.  E.g. GPS or Network") << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>batt</td>\n"
    // Description of 'batt' parameter in table of parameters
    "        <td>" << translate("Numeric remaining battery percentage") << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "      <tr>\n"
    "        <td>note</td>\n"
    // Description of 'note' parameter in table of parameters
    "        <td>" << translate("Free text description related to the specified location") << "</td>\n"
    "        <td>&nbsp;</td>\n"
    "      </tr>\n"
    "    </table>\n"
    "    <p>\n" <<
    // Note displayed beneath the table of location tracking parameters
    translate("<strong>Note:</strong> Where none of the time parameters are included, "
              "the server will record the time it <em>receives</em> the "
              "location using the server's time zone.  As the time received may vary "
              "greatly from the actual time the location was recorded in the device, it "
              "is highly recommended to include a time parameter if at all possible.")
            << "\n</p>\n"
    "  </div>\n";
  os <<
    "  <form name=\"form\" method=\"post\">\n"
    // Text displayed on the button to generate a new tracking ID
    "    <button id=\"btn-generate\" type=\"submit\" name=\"action\" value=\"generate\" class=\"btn btn-lg btn-success\">" << translate("Generate new tracking ID") << "</button>\n"
    "\n"
    "    <div class=\"mt-2\">\n"
    "      <p>"
    // Text about downloading the configuration settings for the TripLogger iOS app
     << translate("If you use <a href=\"https://www.fdsd.co.uk/triplogger/\" "
                  "target=\"_blank\">TripLogger</a> as your tracking client, you can "
                  "download your settings to a file and import the settings into the app "
                  "from the file.  See the section on "
                  "<a href=\"https://www.fdsd.co.uk/trip-server-2/latest/docs/user-guide/Download-TripLogger-Settings.html\" "
                  "target=\"_blank\">Download TripLogger Settings</a> "
                  "in the TripLogger user documentation for more information.")
     << "</p>\n"
    "\n"
    "      <p>\n"
    // Text about uploading the configuration settings for the TripLogger iOS app
     << translate("After <a href=\"account\">uploading your settings</a>, when you subsequently "
                  "download them using this page, they are automatically updated to contain the "
                  "current tracking&nbsp;ID, a potentially easier method than cut-and-paste "
                  "for updating the tracking&nbsp;ID in TripLogger after generating a new tracking&nbsp;ID.")
     << "</p>\n"
    "      <p>\n"
    // Text about using the Traccar Client iOS app the replacement parameter shows a UUID string e.g. 2b3e8d99-0d8b-450a-a716-1ba13c89a129
     << format(translate("For the <a href=\"https://www.traccar.org/client/\" target=\"_blank\">Traccar Client</a> "
                         "app, enter the 'Device identifier' as {1} and "
                         "set the host and port values in the normal way.  As there is no option on "
                         "the app to specify a URL prefix, this tracker client may not be supported "
                         "on some implementations of TRIP.")) % uuid
     << "</p>\n"
    "\n"
    "    </div>\n"
    // Text shown on the button used to download the file containing the TripLogger configuration settings
    "    <button id=\"btn-download\" formaction=\"" << get_uri_prefix() << "/download-triplogger-config\" class=\"btn btn-lg btn-primary\">" << translate("Download TripLogger Settings") << "</button>\n"
    "  </form>\n"
    "  <div class=\"mt-3 pb-2\">\n"
    // Message shown above text showing configuration for the GPSLogger Android app
    "    <p>" << translate("Use the text below to configure GPSLogger") << "</p>\n"
    "    <textarea readonly>" << get_uri_prefix() << "/log_point?lat=%LAT&amp;lon=%LON&amp;time=%TIME&amp;hdop=%ACC&amp;altitude=%ALT&amp;speed=%SPD&amp;bearing=%DIR&amp;sat=%SAT&amp;prov=%PROV&amp;batt=%BATT&amp;note=%DESC&uuid=" << uuid << "</textarea>\n"
    "\n"
    // Message shown above text showing configuration for the OsmAnd Android/iOS app
    "   <p>" << translate("Use the text below to configure OsmAnd") << "</p>\n"
    "   <textarea readonly>" << get_uri_prefix() << "/log_point?lat={0}&amp;lon={1}&amp;mstime={2}&amp;hdop={3}&amp;altitude={4}&amp;speed={5}&amp;bearing={6}&amp;uuid=" << uuid << "</textarea>"
    "  </div>\n"
    "</div>\n";
}

void TrackingInfoHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response) const
{
  bool new_uuid_flag = false;
  const std::string user_id = get_user_id();
  TrackPgDao dao;
  std::string logging_uuid;
  const std::string action = request.get_param("action");
  // std::cout << "Action: \"" << action << "\"\n";
  if (action == "generate") {
    logging_uuid = UUID::generate_uuid();
    new_uuid_flag = true;
    dao.save_logging_uuid(user_id, logging_uuid);
    // std::cout << "Saved new UUID: \"" << logging_uuid << "\"\n";
  } else {
    logging_uuid = dao.get_logging_uuid_by_user_id(user_id);
  }
  build_form(response.content, logging_uuid, new_uuid_flag);
}
