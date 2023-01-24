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
#ifndef ITINERARY_IMPORT_HANLDER_HPP
#define ITINERARY_IMPORT_HANLDER_HPP

#include "itinerary_pg_dao.hpp"
#include "trip_request_handler.hpp"
#include <string>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

/**
 * Handles importing a YAML file containing a full itinerary including
 * itinerary sharing, routes, waypoints and tracks.
 */
class ItineraryImportHandler : public TripAuthenticatedRequestHandler {
  void build_form(web::HTTPServerResponse& response);
protected:
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  ItineraryImportHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config) {}
  virtual ~ItineraryImportHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItineraryImportHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary/import($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryImportHandler>(
        new ItineraryImportHandler(config));
  }
  static long duplicate_itinerary(
      std::string user_id,
      ItineraryPgDao::itinerary_complete& itinerary);
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_IMPORT_HANLDER_HPP
