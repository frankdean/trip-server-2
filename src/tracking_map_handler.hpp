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
#ifndef TRACKING_MAP_HANDLER_HPP
#define TRACKING_MAP_HANDLER_HPP

#include "trip_request_handler.hpp"
#include <string>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

class TrackingMapHandler : public BaseMapHandler {
  static const std::string tracking_map_url;
  std::shared_ptr<TripConfig> config;
protected:
  virtual std::string get_page_title() const override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void append_pre_body_end(std::ostream& os) const override;
public:
  TrackingMapHandler(std::shared_ptr<TripConfig> config);
  virtual ~TrackingMapHandler() {}
  virtual std::string get_handler_name() const override {
    return "TrackingMapHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override;
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<TrackingMapHandler>(
        new TrackingMapHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // TRACKING_MAP_HANDLER_HPP
