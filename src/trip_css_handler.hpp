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
#ifndef TRIP_CSS_HANDLER_HPP
#define TRIP_CSS_HANDLER_HPP

#include "trip_css_handler.hpp"
#include "../trip-server-common/src/http_request_handler.hpp"

namespace fdsd
{
namespace trip
{

class TripCssHandler : public web::CssRequestHandler {
protected:
  virtual void append_stylesheet_content(
      const fdsd::web::HTTPServerRequest& request,
      fdsd::web::HTTPServerResponse& response) const override;
public:
  static const std::string trip_css_url;
  TripCssHandler(std::string uri_prefix) : CssRequestHandler(uri_prefix) {}
  virtual ~TripCssHandler() {}
  virtual std::string get_handler_name() const override {
    return "TripCssHandler";
  }
  virtual std::unique_ptr<fdsd::web::BaseRequestHandler>
      new_instance() const override {
    return std::unique_ptr<TripCssHandler>(
        new TripCssHandler(get_uri_prefix()));
  }
  virtual bool can_handle(
      const fdsd::web::HTTPServerRequest& request) const override {
    return request.uri == get_uri_prefix() + trip_css_url;
  }
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_CSS_HANDLER_HPP
