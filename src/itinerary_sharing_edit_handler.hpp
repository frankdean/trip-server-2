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
#ifndef ITINERARY_SHARING_EDIT_HANDLER
#define ITINERARY_SHARING_EDIT_HANDLER

#include "itinerary_pg_dao.hpp"
#include "trip_request_handler.hpp"
#include <optional>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class ElevationService;

class ItinerarySharingEditHandler :  public TripAuthenticatedRequestHandler {
  std::shared_ptr<ElevationService> elevation_service;
  long itinerary_id;
  std::optional<long> shared_to_id;
  bool is_new;
  bool invalid_nickname_error;
  /// Used to determine which page to return to in the itinerary sharing list
  std::uint32_t current_page;
  /// String used to indicate who the caller to redirect to
  std::string routing;
  /// String containing the page number of the itinerary shares report
  std::string report_page;
  void build_form(web::HTTPServerResponse& response,
                  const ItineraryPgDao::itinerary_share &itinerary_share) const;
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  ItinerarySharingEditHandler(
      std::shared_ptr<TripConfig> config,
      std::shared_ptr<ElevationService> elevation_service) :
    TripAuthenticatedRequestHandler(config),
    elevation_service(elevation_service),
    itinerary_id(),
    shared_to_id(),
    is_new(false),
    invalid_nickname_error(false),
    current_page(1),
    routing(),
    report_page() {}
  virtual ~ItinerarySharingEditHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItinerarySharingEditHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary-sharing/edit[/\\?]?.*");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItinerarySharingEditHandler>(
        new ItinerarySharingEditHandler(config, elevation_service));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_SHARING_EDIT_HANDLER
