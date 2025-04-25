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
#ifndef PASSWORD_CHANGE_HANLDER_HPP
#define PASSWORD_CHANGE_HANLDER_HPP

#include "trip_request_handler.hpp"
#include <boost/locale.hpp>
#include <ostream>
#include <string>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

class PasswordChangeHandler : public TripAuthenticatedRequestHandler {
  void build_form(std::ostream &os);
  /// Flag set when original password fails validation
  bool bad_credentials;
  /// Flag set when the two new passwords submitted do not match
  bool passwords_match_failure;
  bool week_password;
protected:
  virtual void append_pre_body_end(std::ostream& os) const override;
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  PasswordChangeHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config),
    bad_credentials(),
    passwords_match_failure(),
    week_password() {}
  virtual std::string get_handler_name() const override {
    return "PasswordChangeHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/change-password($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<PasswordChangeHandler>(
        new PasswordChangeHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // PASSWORD_CHANGE_HANLDER_HPP
