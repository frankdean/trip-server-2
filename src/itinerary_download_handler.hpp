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
#ifndef ITINERARY_DOWNLOAD_HANDLER_HPP
#define ITINERARY_DOWNLOAD_HANDLER_HPP

#include "itinerary_pg_dao.hpp"
#include "trip_request_handler.hpp"
#include <pugixml.hpp>
#include <string>

namespace fdsd {
namespace trip {

class ItineraryDownloadHandler : public BaseRestHandler {
  long itinerary_id;
  enum download_format {
    gpx,
    kml
  };
  download_format download_type;
  void handle_gpx_download(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response);
  void append_xml(pugi::xml_node &target, const std::string &xml);
  void append_kml_route_styles(pugi::xml_node &target);
  void append_kml_track_styles(pugi::xml_node &target);
  void append_kml_styles(pugi::xml_node &target);
  void append_kml_waypoints(
      pugi::xml_node &kml, const std::vector<ItineraryPgDao::waypoint> &waypoints);
  void append_kml_routes(
      pugi::xml_node &kml, std::vector<ItineraryPgDao::route> &routes);
  void append_kml_tracks(
      pugi::xml_node &kml, std::vector<ItineraryPgDao::track> &tracks);
  void handle_kml_download(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response);
protected:
  virtual void set_content_headers(web::HTTPServerResponse& response) const override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  ItineraryDownloadHandler(std::shared_ptr<TripConfig> config) :
    BaseRestHandler(config),
    itinerary_id(),
    download_type(gpx) {}
  virtual ~ItineraryDownloadHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItineraryDownloadHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary/download($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryDownloadHandler>(
        new ItineraryDownloadHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_DOWNLOAD_HANDLER_HPP
