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
#ifndef ITINERARY_WAYPOINT_EDIT
#define ITINERARY_WAYPOINT_EDIT

#include "trip_request_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include <vector>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class ItineraryWaypointEditHandler : public TripAuthenticatedRequestHandler {
  bool read_only;
  long itinerary_id;
  std::pair<bool, long> waypoint_id;
  /// The format control code for displaying waypoint coordinates in e.g. '%i%d'
  std::string coord_format;
  /// The formatting code for ordering waypoint coordinates in, e.g. 'lat,lng'
  std::string position_format;
  void build_form(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response,
      ItineraryPgDao::waypoint* waypoint,
      const std::vector<std::pair<std::string, std::string>> &georef_formats,
      const std::vector<std::pair<std::string, std::string>> &waypoint_symbols);
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void append_pre_body_end(std::ostream& os) const override;
public:
  ItineraryWaypointEditHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config),
    read_only(true),
    itinerary_id(),
    waypoint_id(),
    coord_format(),
    position_format("lat,lng") {}
  virtual ~ItineraryWaypointEditHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItineraryWaypointEditHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary-wpt-edit($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryWaypointEditHandler>(
        new ItineraryWaypointEditHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_WAYPOINT_EDIT
