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
#ifndef ITINERARY_PATH_EDIT_HANDLER_HPP
#define ITINERARY_PATH_EDIT_HANDLER_HPP

#include "itinerary_pg_dao.hpp"
#include "trip_request_handler.hpp"
#include <string>
#include <ostream>
#include <vector>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

/**
 * Handles editing the name and color attributes of a path (route or track).
 */
class ItineraryPathNameEdit : public TripAuthenticatedRequestHandler {
  void build_form(std::ostream &os);
protected:
  long itinerary_id;
  long path_id;
  /// The route or track name
  std::pair<bool, std::string> name;
  /// The route or track color key
  std::pair<bool, std::string> color_key;
  /// Option selecting to make a copy of the route
  bool make_copy;
  /// Option selecting to make a reversed copy of the route
  bool reverse_route;
  /// The total distance of the path or route
  std::pair<bool, double> distance;
  /// The available colors for selection
  std::vector<std::pair<std::string, std::string>> colors;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void load_path_data(
      const web::HTTPServerRequest &request, ItineraryPgDao &dao) = 0;
  virtual void insert_extra_form_controls(std::ostream &os) {}
  virtual void save_path(ItineraryPgDao &dao) = 0;
public:
  ItineraryPathNameEdit(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config),
    name(),
    color_key(),
    make_copy(false),
    distance(),
    colors()
    {}
  virtual ~ItineraryPathNameEdit() {}
  virtual std::string get_handler_name() const override {
    return "ItineraryPathNameEdit";
  }
};

class ItineraryRouteNameEdit : public ItineraryPathNameEdit {
//  ItineraryPgDao::route route;
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void load_path_data(
      const web::HTTPServerRequest &request, ItineraryPgDao &dao) override;
  virtual void insert_extra_form_controls(std::ostream &os) override;
  virtual void save_path(ItineraryPgDao &dao) override;
public:
  ItineraryRouteNameEdit(std::shared_ptr<TripConfig> config) :
    ItineraryPathNameEdit(config) {}
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary.route.name($|\\?.*)");
  }
  virtual std::string get_handler_name() const override {
    return "ItineraryRouteNameEdit";
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryRouteNameEdit>(
        new ItineraryRouteNameEdit(config));
  }
};

class ItineraryTrackNameEdit : public ItineraryPathNameEdit {
//  ItineraryPgDao::route track;
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void load_path_data(
      const web::HTTPServerRequest &request, ItineraryPgDao &dao) override;
  virtual void save_path(ItineraryPgDao &dao) override;
public:
  ItineraryTrackNameEdit(std::shared_ptr<TripConfig> config) :
    ItineraryPathNameEdit(config) {}
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary.track.name($|\\?.*)");
  }
  virtual std::string get_handler_name() const override {
    return "ItineraryTrackNameEdit";
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryTrackNameEdit>(
        new ItineraryTrackNameEdit(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_PATH_EDIT_HANDLER_HPP
