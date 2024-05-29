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
#ifndef TRACK_SHARING_HANDLER_HPP
#define TRACK_SHARING_HANDLER_HPP

#include "tracking_pg_dao.hpp"
#include "trip_request_handler.hpp"

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class ElevationService;

class TrackSharingHandler :  public TripAuthenticatedRequestHandler {
  std::shared_ptr<ElevationService> elevation_service;
  void build_form(web::HTTPServerResponse& response,
                  const web::Pagination& pagination,
                  const std::vector<TrackPgDao::track_share>& track_shares) const;
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  TrackSharingHandler(std::shared_ptr<TripConfig> config,
                      std::shared_ptr<ElevationService> elevation_service) :
    TripAuthenticatedRequestHandler(config),
    elevation_service(elevation_service) {}
  virtual ~TrackSharingHandler() {}
  virtual std::string get_handler_name() const override {
    return "TrackSharingHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    // return compare_request_regex(request.uri, "/sharing[^/]*");
    return compare_request_regex(request.uri, "/sharing(\\?page=[0-9]*)?$");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<TrackSharingHandler>(
        new TrackSharingHandler(config, elevation_service));
  }
};

struct period_dhm {
  int days;
  int hours;
  int minutes;
  period_dhm(int days, int hours, int minutes) :
    days(days), hours(hours), minutes(minutes) {}
  period_dhm(int minutes) {
    hours = minutes / 60;
    this->minutes = minutes - hours * 60;
    days = hours / 24;
    hours = hours - days * 24;
  }
  int get_total_minutes() const {
    return minutes + (hours + days * 24) * 60;
  }
  std::string to_string() const {
    return std::to_string(days) + "d "
      + std::to_string(hours) + "h "
      + std::to_string(minutes) + "m";
  }
};

} // namespace trip
} // namespace fdsd

#endif // TRACK_SHARING_HANDLER_HPP
