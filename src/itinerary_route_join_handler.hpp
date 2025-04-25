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
#ifndef ITINERARY_ROUTE_JOIN_HANDLER_HPP
#define ITINERARY_ROUTE_JOIN_HANDLER_HPP

#include "itinerary_path_join_handler.hpp"
#include "trip_request_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include <string>
#include <vector>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class ItineraryRouteJoinHandler : public ItineraryPathJoinHandler {
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual std::vector<ItineraryPgDao::path_summary>
      get_paths(ItineraryPgDao &dao) override;
  virtual void join_paths(ItineraryPgDao &dao, std::vector<long> ids) override;
  virtual ItineraryPgDao::selected_feature_ids get_selected_features() override;
public:
  ItineraryRouteJoinHandler(std::shared_ptr<TripConfig> config) :
    ItineraryPathJoinHandler(config) {}
  virtual std::string get_handler_name() const override {
    return "ItineraryRouteJoinHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary.route.join($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryRouteJoinHandler>(
        new ItineraryRouteJoinHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_ROUTE_JOIN_HANDLER_HPP
