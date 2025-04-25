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
#ifndef ITINERARY_SEARCH_RESULTS_HANDLER_HPP
#define ITINERARY_SEARCH_RESULTS_HANDLER_HPP

#include "trip_request_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include "../trip-server-common/src/pagination.hpp"
#include <boost/locale.hpp>
#include <optional>
#include <ostream>
#include <string>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

class ItinerarySearchResultsHandler : public TripAuthenticatedRequestHandler {
  double longitude;
  double latitude;
  double radius;
  std::optional<std::string> error_message;
  void build_form(
      std::ostream &os,
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
  ItinerarySearchResultsHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config),
    longitude(),
    latitude(),
    radius() {}
  static const int max_search_radius_kilometers;
  virtual std::string get_handler_name() const override {
    return "ItinerarySearchResultsHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary-search-result($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItinerarySearchResultsHandler>(
        new ItinerarySearchResultsHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_SEARCH_RESULTS_HANDLER_HPP
