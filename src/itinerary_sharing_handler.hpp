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
#ifndef ITINERARY_SHARING_HANDLER_HPP
#define ITINERARY_SHARING_HANDLER_HPP

#include "itinerary_pg_dao.hpp"
#include "trip_request_handler.hpp"

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class ItinerarySharingHandler :  public TripAuthenticatedRequestHandler {

  long itinerary_id;

  /// String used to indicate who the caller to redirect to
  std::string routing;

  /// String containing the page number of the itinerary shares report
  std::string report_page;

  void build_form(web::HTTPServerResponse& response,
                const web::Pagination& pagination,
                const std::vector<ItineraryPgDao::itinerary_share>& itinerary_shares) const;

protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  ItinerarySharingHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config),
    itinerary_id(),
    routing(),
    report_page() {}
  virtual ~ItinerarySharingHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItinerarySharingHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary-sharing/?\\?.*");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItinerarySharingHandler>(
        new ItinerarySharingHandler(config));
  }
};

} // namespace trip
} // namespace fdsd


#endif // ITINERARY_SHARING_HANDLER_HPP
