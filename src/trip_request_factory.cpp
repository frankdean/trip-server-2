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
#include "../config.h"
#include "trip_request_factory.hpp"
#include "tile_handler.hpp"
#include "trip_config.hpp"
#include "track_logging_handler.hpp"
#include "tracking_download_handler.hpp"
#include "tracking_map_handler.hpp"
#include "tracking_request_handler.hpp"
#include "tracking_rest_handler.hpp"
// #include "trip_css_handler.hpp"
#include "trip_request_handler.hpp"
// #include "trip_application.hpp"
#include "../trip-server-common/src/http_request.hpp"
#include <assert.h>
#include <cstring>
#include <iostream>
#include <utility>

using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;

Logger TripRequestFactory::logger("TripRequestFactory", std::clog, Logger::info);

TripRequestFactory::TripRequestFactory(std::shared_ptr<TripConfig> config)
  : HTTPRequestFactory(config->get_application_prefix_url())
{
  // pre_login_handlers.push_back(
  //     std::make_shared<TripCssHandler>(
  //         TripCssHandler(get_uri_prefix())));
  pre_login_handlers.push_back(
      std::make_shared<TripLogoutRequestHandler>(
          TripLogoutRequestHandler(get_uri_prefix())));
  // pre_login_handlers.push_back(
  //     std::make_shared<TripRequestHandler>(
  //         TripRequestHandler(get_uri_prefix())));
#ifdef ALLOW_STATIC_FILES
  pre_login_handlers.push_back(
      std::make_shared<FileRequestHandler>(
          FileRequestHandler(get_uri_prefix(),
                             config->get_root_directory())));
#endif
  pre_login_handlers.push_back(
      std::make_shared<TrackLoggingHandler>(
          TrackLoggingHandler(config)));
  post_login_handlers.push_back(
      std::make_shared<TrackingMapHandler>(
          TrackingMapHandler(config)));
  post_login_handlers.push_back(
      std::make_shared<TrackingRequestHandler>(
          TrackingRequestHandler(config)));
  post_login_handlers.push_back(
      std::make_shared<TrackingRestHandler>(
          TrackingRestHandler(config)));
  post_login_handlers.push_back(
      std::make_shared<TrackingDownloadHandler>(
          TrackingDownloadHandler(config)));
  post_login_handlers.push_back(
      std::make_shared<TileHandler>(
          TileHandler(config)));
  post_login_handlers.push_back(
      std::make_shared<TripAuthenticatedRequestHandler>(
          TripAuthenticatedRequestHandler(config)));
}

std::string TripRequestFactory::get_session_id_cookie_name() const
{
  return TripLoginRequestHandler::session_id_cookie_name;
}

bool TripRequestFactory::is_login_uri(std::string uri) const
{
  return uri.find(TripLoginRequestHandler::login_url) != std::string::npos;
}

std::unique_ptr<HTTPRequestHandler>
    TripRequestFactory::get_login_handler() const
{
  return std::unique_ptr<HTTPRequestHandler>(
      new TripLoginRequestHandler(get_uri_prefix()));
}

bool TripRequestFactory::is_logout_uri(std::string uri) const
{
  return uri.find(TripLogoutRequestHandler::logout_url) != std::string::npos;
}

std::unique_ptr<HTTPRequestHandler>
    TripRequestFactory::get_logout_handler() const
{
  return std::unique_ptr<HTTPRequestHandler>(
      new TripLogoutRequestHandler(get_uri_prefix()));
}

std::unique_ptr<HTTPRequestHandler>
    TripRequestFactory::get_not_found_handler() const
{
  return std::unique_ptr<HTTPRequestHandler>(
      new TripNotFoundHandler(get_uri_prefix()));
}

std::string TripRequestFactory::get_user_id(std::string session_id) const
{
  if (!session_id.empty())
    return SessionManager::get_session_manager()->get_session_user_id(session_id);

  return "";
}

bool TripRequestFactory::is_application_prefix_uri(std::string uri) const
{
  bool retval = !uri.empty() && uri.find(get_uri_prefix()) == 0;
  if (logger.is_level(Logger::debug))
    logger << Logger::debug
           << "The expected prefix for application URLs is \""
           << get_uri_prefix() << "\".  "
           << "The URL \"" << uri << "\" is " << (retval ? "" : "not ")
           << "an application URL" << Logger::endl;
  return retval;
}

bool TripRequestFactory::is_valid_session(std::string session_id,
                                          std::string user_id) const
{
  std::pair<bool, std::string> user =
    SessionManager::get_session_manager()->get_user_id_for_session(session_id);
  bool retval = user.first && user.second == user_id;
  if (logger.is_level(Logger::debug))
    logger << Logger::debug
           << "The session for user ID \"" << user_id << "\" is "
           << (retval ? "" : "not ")
           << "valid for session ID: \"" << session_id << "\""
           << Logger::endl;
  return retval;
}
