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
#ifndef ITINERARIES_HANDLER_HPP
#define ITINERARIES_HANDLER_HPP

#include "trip_request_handler.hpp"
#include "itinerary_pg_dao.hpp"

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class ItinerariesHandler : public TripAuthenticatedRequestHandler {
  bool itinerary_upload_failed;
  void build_page(
    web::HTTPServerResponse& response,
    const web::Pagination& pagination,
    const std::vector<ItineraryPgDao::itinerary_summary> itineraries);
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  ItinerariesHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config),
    itinerary_upload_failed() {}
  virtual ~ItinerariesHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItinerariesHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itineraries($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItinerariesHandler>(
        new ItinerariesHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARIES_HANDLER_HPP
