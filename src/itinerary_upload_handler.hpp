// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2024 Frank Dean <frank.dean@fdsd.co.uk>

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
#ifndef ITINERARY_UPLOAD_HANLDER_HPP
#define ITINERARY_UPLOAD_HANLDER_HPP

#include "itinerary_pg_dao.hpp"
#include "trip_request_handler.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include <optional>
#include <pugixml.hpp>
#include <string>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

class ElevationService;

/**
 * Handles uploading a GPX (XML) file containing routes, waypoints and
 * tracks.
 */
class ItineraryUploadHandler : public TripAuthenticatedRequestHandler {
  std::shared_ptr<ElevationService> elevation_service;
  long itinerary_id;
  void build_form(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response);
  void add_waypoint(
      ItineraryPgDao::itinerary_features &features,
      const pugi::xml_node &node);
  void add_route_point(
      ItineraryPgDao::route &route,
      const pugi::xml_node &node);
  void add_route(
      ItineraryPgDao::itinerary_features &features,
      const pugi::xml_node &node);
  void add_track_point(
      ItineraryPgDao::track_segment &track_segment,
      const pugi::xml_node &node);
  void add_track_segment(
      ItineraryPgDao::track &track,
      const pugi::xml_node &node);
  void add_track(
      ItineraryPgDao::itinerary_features &features,
      const pugi::xml_node &node);
  void save(const pugi::xml_document &doc);
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  static std::optional<std::string> child_node_as_string(
      const pugi::xml_node &node, const pugi::char_t *child_name);
  static std::optional<double> child_node_as_double(
      const pugi::xml_node &node, const pugi::char_t *child_name);
  static std::optional<float> child_node_as_float(
      const pugi::xml_node &node, const pugi::char_t *child_name);
  static std::optional<long> child_node_as_long(
      const pugi::xml_node &node, const pugi::char_t *child_name);
  static std::optional<std::chrono::system_clock::time_point>
      child_node_as_time_point(const pugi::xml_node &node,
                               const pugi::char_t *child_name);
public:
  ItineraryUploadHandler(std::shared_ptr<TripConfig> config,
                         std::shared_ptr<ElevationService> elevation_service) :
    TripAuthenticatedRequestHandler(config),
    elevation_service(elevation_service),
    itinerary_id() {}
  virtual ~ItineraryUploadHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItineraryUploadHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary/upload($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryUploadHandler>(
        new ItineraryUploadHandler(config, elevation_service));
  }
  static std::string xml_osmand_namespace;
  static std::string xml_osmand_color;

};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_UPLOAD_HANLDER_HPP
