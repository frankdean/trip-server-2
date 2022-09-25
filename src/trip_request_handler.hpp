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
#ifndef TRIP_REQUEST_HANDLER_HPP
#define TRIP_REQUEST_HANDLER_HPP

#include "../trip-server-common/src/http_request_handler.hpp"
#include "../trip-server-common/src/session.hpp"

namespace fdsd
{
namespace trip
{

class TripConfig;

class TripRequestHandler : public web::HTTPRequestHandler {
protected:
  std::shared_ptr<TripConfig> config;
  virtual std::string get_page_title() const override {
    return "Hello World!";
  }
public:
  static const std::string default_url;
  static const std::string success_url;
  TripRequestHandler(std::shared_ptr<TripConfig> config);
  virtual std::string get_handler_name() const override {
    return "TripRequestHandler";
  }
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const override;
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    std::string prefix = get_uri_prefix();
    // Act as default handler for the application root URL or URLs that do not
    // start with the root.
    return request.uri == prefix ||
      (prefix.length() > 1 && request.uri.find(prefix) != 0);
  }
  virtual void do_handle_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) const override;
};

class TripLoginRequestHandler : public fdsd::web::HTTPLoginRequestHandler {
protected:
  virtual std::string get_page_title() const override;
  virtual bool validate_password(const std::string email,
                                 const std::string password) const override;
  virtual std::string get_user_id_by_email(
      const std::string email) const override;
  virtual std::string get_default_uri() const override {
    return get_uri_prefix();
  }
  virtual std::string get_login_uri() const override {
    return get_uri_prefix() + TripLoginRequestHandler::login_url;
  }
  virtual std::string get_session_id_cookie_name() const override {
    return TripLoginRequestHandler::session_id_cookie_name;
  }
  virtual web::SessionManager* get_session_manager() const override {
    return web::SessionManager::get_session_manager();
  }
  virtual std::string get_login_redirect_cookie_name() const override {
    return TripLoginRequestHandler::login_redirect_cookie_name;
  }
public:
  static const std::string login_url;
  static const std::string login_redirect_cookie_name;
  static const std::string session_id_cookie_name;
  static const std::string test_user_id;
  static const std::string test_username;
  static const std::string test_password;
  TripLoginRequestHandler(std::string uri_prefix) :
    fdsd::web::HTTPLoginRequestHandler(uri_prefix) {
  }
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<TripLoginRequestHandler>(
        new TripLoginRequestHandler(get_uri_prefix()));
  }
  virtual bool can_handle(const web::HTTPServerRequest& request) const override {
    return request.uri == get_uri_prefix() +
      TripLoginRequestHandler::login_url;
  }
  virtual std::string get_handler_name() const override {
    return "TripLoginRequestHandler";
  }
};

class TripLogoutRequestHandler : public fdsd::web::HTTPLogoutRequestHandler {
protected:
  virtual std::string get_login_uri() const override {
    return get_uri_prefix() + TripLoginRequestHandler::login_url;
  }
  virtual std::string get_default_uri() const override {
    return get_login_uri();
  }
  virtual std::string get_session_id_cookie_name() const override {
    return TripLoginRequestHandler::session_id_cookie_name;
  }
  virtual web::SessionManager* get_session_manager() const override {
    return web::SessionManager::get_session_manager();
  }
  virtual std::string get_login_redirect_cookie_name() const override {
    return TripLoginRequestHandler::login_redirect_cookie_name;
  }
  virtual std::string get_page_title() const override {
    return "Logout";
  }
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<TripLogoutRequestHandler>(
        new TripLogoutRequestHandler(get_uri_prefix()));
  }
  virtual bool can_handle(const web::HTTPServerRequest& request) const override {
    return request.uri == get_uri_prefix() +
      TripLogoutRequestHandler::logout_url;
  }
  virtual std::string get_handler_name() const override {
    return "TripLogoutRequestHandler";
  }
public:
  static const std::string logout_url;
  TripLogoutRequestHandler(std::string uri_prefix) :
    fdsd::web::HTTPLogoutRequestHandler(uri_prefix) {}
};

class TripNotFoundHandler : public web::HTTPNotFoundRequestHandler {
protected:
  virtual std::string get_default_uri() const override {
    return get_uri_prefix() + TripRequestHandler::default_url;
  }
public:
  TripNotFoundHandler(std::string uri_prefix) :
    HTTPNotFoundRequestHandler(uri_prefix) {}
};

class TripAuthenticatedRequestHandler : public web::AuthenticatedRequestHandler {
protected:
  std::shared_ptr<TripConfig> config;
  enum menu_items { unknown, tracks };
private:
    menu_items menu_item;
protected:
  virtual std::string get_page_title() const override {
    return "Login Success!";
  }
  virtual std::string get_login_uri() const override {
    return get_uri_prefix() + TripLoginRequestHandler::login_url;
  }
  virtual std::string get_session_id_cookie_name() const override {
    return TripLoginRequestHandler::session_id_cookie_name;
  }
  virtual web::SessionManager* get_session_manager() const override {
    return web::SessionManager::get_session_manager();
  }
  virtual std::string get_login_redirect_cookie_name() const override {
    return TripLoginRequestHandler::login_redirect_cookie_name;
  }
  virtual std::string get_default_uri() const override {
    return get_uri_prefix() + "/tracks";
  }
  virtual void append_head_content(std::ostream& os) const override;
  virtual void append_head_title_section(std::ostream& os) const override;
  virtual void append_body_start(std::ostream& os) const override;
  virtual void append_pre_body_end(std::ostream& os) const override;
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override {}
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) const override;
  virtual void append_bootstrap_scripts(std::ostream& os) const;
  virtual void append_openlayers_scripts(std::ostream& os) const;
  virtual void append_footer_content(std::ostream& os) const override;
  void set_menu_item(menu_items item) {
    menu_item = item;
  }
  menu_items get_menu_item() const {
    return menu_item;
  }
public:
  TripAuthenticatedRequestHandler(std::shared_ptr<TripConfig> config);
  virtual std::string get_handler_name() const override {
    return "TripAuthenticatedRequestHandler";
  }
  // virtual void handle_request(
  //     const web::HTTPServerRequest& request,
  //     web::HTTPServerResponse& response) override;
  virtual bool can_handle(const web::HTTPServerRequest& request) const override {
    return request.uri == get_uri_prefix() + TripRequestHandler::success_url;
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<TripAuthenticatedRequestHandler>(
        new TripAuthenticatedRequestHandler(config));
  }
};

class BaseRestHandler : public TripAuthenticatedRequestHandler {
protected:
  virtual void append_doc_type(std::ostream& os) const override {}
  virtual void append_html_start(std::ostream& os) const override {}
  virtual void append_head_start(std::ostream& os) const override {}
  virtual void append_head_section(std::ostream& os) const override {}
  virtual void append_head_title_section(std::ostream& os) const override {}
  virtual void append_head_content(std::ostream& os) const override {}
  virtual void append_head_end(std::ostream& os) const override {}
  virtual void append_body_start(std::ostream& os) const override {}
  virtual void append_header_content(std::ostream& os) const override {}
  virtual void append_footer_content(std::ostream& os) const override {}
  virtual void append_pre_body_end(std::ostream& os) const override {}
  virtual void append_body_end(std::ostream& os) const override {}
  virtual void append_html_end(std::ostream& os) const override {}
  virtual void set_content_headers(web::HTTPServerResponse& response) const override;
public:
  BaseRestHandler(std::shared_ptr<TripConfig> config);
};

class BaseMapHandler : public TripAuthenticatedRequestHandler {
protected:
  virtual void append_head_content(std::ostream& os) const override;
  virtual void append_pre_body_end(std::ostream& os) const override;
public:
  BaseMapHandler(std::shared_ptr<TripConfig> config);
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_REQUEST_HANDLER_HPP
