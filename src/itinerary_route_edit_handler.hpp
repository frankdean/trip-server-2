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
#ifndef ITINERARY_ROUTE_EDIT_HANDLER_HPP
#define ITINERARY_ROUTE_EDIT_HANDLER_HPP

#include "trip_request_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include <string>
// #include <ostream>
#include <vector>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class ItineraryRouteEditHandler : public BaseMapHandler {
  long itinerary_id;
  long route_id;
  bool read_only;
  bool select_all;
  /// Action parameter
  std::string action;
  /// Map of selected points
  std::map<long, std::string> selected_point_id_map;
  /// IDs of selected points
  std::vector<long> selected_point_ids;
  void build_form(std::ostream &os,
                  const web::Pagination& pagination,
                  const ItineraryPgDao::route &route);
  void delete_points(
      const web::HTTPServerRequest& request,
      ItineraryPgDao &dao);
  void split_route(
      const web::HTTPServerRequest& request,
      ItineraryPgDao &dao);
  void reverse_route(
      const web::HTTPServerRequest& request,
      ItineraryPgDao &dao);
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void append_pre_body_end(std::ostream& os) const override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  ItineraryRouteEditHandler(std::shared_ptr<TripConfig> config) :
    BaseMapHandler(config),
    itinerary_id(),
    route_id(),
    read_only(),
    select_all(),
    action(),
    selected_point_id_map(),
    selected_point_ids() {}
  virtual ~ItineraryRouteEditHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItineraryRouteEditHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary.route.edit($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryRouteEditHandler>(
        new ItineraryRouteEditHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_ROUTE_EDIT_HANDLER_HPP
