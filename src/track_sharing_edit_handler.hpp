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
#ifndef TRACK_SHARING_EDIT_HANDLER_HPP
#define TRACK_SHARING_EDIT_HANDLER_HPP

#include "tracking_pg_dao.hpp"
#include "trip_request_handler.hpp"
#include <ostream>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class TrackSharingEditHandler : public TripAuthenticatedRequestHandler {
  bool is_new;
  std::string nickname;
  int convert_dhm_to_minutes(
      std::string days,
      std::string hours,
      std::string minutes) const;
  void build_form(web::HTTPServerResponse& response,
                  const TrackPgDao::location_share_details& share) const;
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  TrackSharingEditHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config), is_new(false), nickname() {}
  virtual std::string get_handler_name() const override {
    return "TrackSharingEditHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_url(request.uri, "/sharing/edit");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<TrackSharingEditHandler>(
        new TrackSharingEditHandler(config));
  }
};

} // namespace trip
} // namespace fdsd
  
#endif // TRACK_SHARING_EDIT_HANDLER_HPP
