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
#ifndef TILE_HANDLER_HPP
#define TILE_HANDLER_HPP

#include "tile_pg_dao.hpp"
#include "trip_request_handler.hpp"
// #include "../trip-server-common/src/http_client.hpp"
#include <iostream>
#include <regex>
#include <string>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

struct tile_provider;

class TileHandler : public fdsd::trip::BaseRestHandler {
  static const std::regex tile_url_re;
  tile_provider get_provider(int provider_index) const;
  TilePgDao::tile_result fetch_remote_tile(
      int provider_index,
      int z,
      int x,
      int y) const;
  TilePgDao::tile_result find_tile(
      int provider_index,
      int z,
      int x,
      int y) const;
protected:
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  TileHandler(std::shared_ptr<TripConfig> config);
  virtual ~TileHandler() {}

  class tile_not_found_exception : public std::exception {
  private:
    std::string message;
  public:
    tile_not_found_exception(std::string message) {
      this->message = message;
    }
    virtual const char* what() const throw() override {
      return message.c_str();
    }
  };

  virtual std::string get_handler_name() const override {
    return "TileHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override;
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<TileHandler>(
        new TileHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // TILE_HANDLER_HPP
