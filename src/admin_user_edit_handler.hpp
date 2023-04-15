// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2023 Frank Dean <frank.dean@fdsd.co.uk>

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
#ifndef ADMIN_USER_EDIT_HANDLER_HPP
#define ADMIN_USER_EDIT_HANDLER_HPP

#include "trip_request_handler.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/pagination.hpp"
#include <boost/locale.hpp>
#include <ostream>
#include <string>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
}
namespace trip {

class AdminUserEditHandler : public TripAuthenticatedRequestHandler {
  std::string user_id;
  bool user_not_found;
  bool passwords_match_failure;
  bool new_user_flag;
  bool save_failed_flag;
  void build_form(std::ostream &os,
                  fdsd::trip::SessionPgDao::user);
  void save(const web::HTTPServerRequest& request,
            web::HTTPServerResponse& response,
            SessionPgDao &session_dao);
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  AdminUserEditHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config),
    user_id(),
    user_not_found(),
    passwords_match_failure(),
    new_user_flag(),
    save_failed_flag() {}
  virtual std::string get_handler_name() const override {
    return "AdminUserEditHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/(user.edit|edit.user)($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<AdminUserEditHandler>(
        new AdminUserEditHandler(config));
  }
};

} // namespace trip
} // namespace fdsd
  
#endif // ADMIN_USER_EDIT_HANDLER_HPP
