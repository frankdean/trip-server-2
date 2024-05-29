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
#ifndef ADMIN_USER_MANAGEMENT_HANDLER_HPP
#define ADMIN_USER_MANAGEMENT_HANDLER_HPP

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

class AdminUserManagementHandler : public TripAuthenticatedRequestHandler {
  bool empty_search_result;
  bool delete_users_failed_flag;
  std::string email;
  std::string nickname;
  SessionPgDao::search_type search_type;
  void build_form(std::ostream &os,
                  web::Pagination &pagination,
                  std::vector<fdsd::trip::SessionPgDao::user> users);
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  AdminUserManagementHandler(std::shared_ptr<TripConfig> config) :
    TripAuthenticatedRequestHandler(config),
    empty_search_result(),
    delete_users_failed_flag(),
    email(),
    nickname(),
    search_type(SessionPgDao::partial) {}
  virtual std::string get_handler_name() const override {
    return "AdminUserManagementHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/users($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<AdminUserManagementHandler>(
        new AdminUserManagementHandler(config));
  }
};

} // namespace trip
} // namespace fdsd

#endif // ADMIN_USER_MANAGEMENT_HANDLER_HPP
