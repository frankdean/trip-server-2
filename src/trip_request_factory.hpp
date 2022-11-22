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
#ifndef TRIP_REQUEST_FACTORY_HPP
#define TRIP_REQUEST_FACTORY_HPP

#include "../trip-server-common/src/http_request_factory.hpp"

namespace fdsd
{
namespace trip
{

class TripConfig;

class TripRequestFactory : public web::HTTPRequestFactory {
private:
  static fdsd::utils::Logger logger;
public:
  TripRequestFactory(std::shared_ptr<TripConfig> config);
  virtual ~TripRequestFactory() {}
protected:
  virtual std::string get_session_id_cookie_name() const override;
  virtual std::string get_user_id(std::string session_id) const override;
  virtual bool is_login_uri(std::string uri) const override;
  virtual std::unique_ptr<web::HTTPRequestHandler> get_login_handler() const override;
  virtual bool is_logout_uri(std::string uri) const override;
  virtual std::unique_ptr<web::HTTPRequestHandler> get_logout_handler() const override;
  virtual bool is_application_prefix_uri(std::string uri) const override;
  virtual std::unique_ptr<web::HTTPRequestHandler> get_not_found_handler() const override;
  virtual bool is_valid_session(std::string session_id, std::string user_id) const override;
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_REQUEST_FACTORY_HPP
