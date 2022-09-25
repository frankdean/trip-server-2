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
#ifndef TRACKING_REQUEST_HANDLER_HPP
#define TRACKING_REQUEST_HANDLER_HPP

#include "tracking_pg_dao.hpp"
#include "trip_request_handler.hpp"
#include "../trip-server-common/src/pagination.hpp"
#include <string>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

class TrackingRequestHandler : public TripAuthenticatedRequestHandler {
private:
  static const std::string tracking_url;
  void build_form(
      web::HTTPServerResponse& response,
      bool first_time,
      const fdsd::web::Pagination& pagination,
      TrackPgDao::location_search_query_params query_params,
      TrackPgDao::nickname_result nickname_result,
      TrackPgDao::tracked_locations_result locations_result) const;
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) const override;
public:
  TrackingRequestHandler(std::shared_ptr<TripConfig> config);
  virtual std::string get_handler_name() const override {
    return "TrackingRequestHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    const std::string wanted_url = get_uri_prefix() + tracking_url;
    return !request.uri.empty() &&
      request.uri.compare(0, wanted_url.length(), wanted_url) == 0;
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<TrackingRequestHandler>(
        new TrackingRequestHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // TRACKING_REQUEST_HANDLER_HPP
