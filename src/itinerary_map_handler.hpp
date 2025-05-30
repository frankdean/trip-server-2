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
#ifndef ITINERARY_MAP_HANDLER_HPP
#define ITINERARY_MAP_HANDLER_HPP

#include "trip_request_handler.hpp"
#include <string>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

class ElevationService;

class ItineraryMapHandler : public BaseMapHandler {
  long itinerary_id;
  std::shared_ptr<ElevationService> elevation_service;
protected:
  virtual std::string get_page_title() const override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void append_pre_body_end(std::ostream& os) const override;
public:
  ItineraryMapHandler(std::shared_ptr<TripConfig> config,
                      std::shared_ptr<ElevationService> elevation_service) :
    BaseMapHandler(config),
    itinerary_id(),
    elevation_service(elevation_service) {}
  virtual ~ItineraryMapHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItineraryMapHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override;
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryMapHandler>(
        new ItineraryMapHandler(config, elevation_service));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_MAP_HANDLER_HPP
