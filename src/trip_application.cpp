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
#include "trip_application.hpp"
#ifdef HAVE_GDAL
#include "elevation_tile.hpp"
#endif
#include "trip_request_factory.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/session.hpp"
#include <chrono>

using namespace fdsd::utils;
using namespace fdsd::web;
using namespace fdsd::trip;

#ifdef HAVE_GDAL
extern ElevationService *elevation_service;
#endif

TripApplication::TripApplication(std::string listen_address,
                                 std::string port,
                                 std::string config_filename,
                                 std::string locale) :
  Application(
    listen_address,
    port,
    locale)
{
  config = std::make_shared<TripConfig>(TripConfig(config_filename));
#ifdef HAVE_GDAL
  if (!config->get_elevation_tile_path().empty()) {
    elevation_service = new ElevationService(
        config->get_elevation_tile_path(),
        config->get_elevation_tile_cache_ms());
  }
#endif
}

TripApplication::~TripApplication()
{
#ifdef HAVE_GDAL
  if (elevation_service != nullptr)
    delete elevation_service;
#endif
}

std::string TripApplication::get_db_connect_string() const {
  return config->get_db_connect_string();
}
int TripApplication::get_worker_count() const {
  return config->get_worker_count();
}
int TripApplication::get_pg_pool_size() const {
  return config->get_pg_pool_size();
}
std::string TripApplication::get_application_prefix_url() const {
  return config->get_application_prefix_url();
}

void TripApplication::set_root_directory(std::string directory) {
  config->set_root_directory(directory);
}

std::shared_ptr<HTTPRequestFactory>
    TripApplication::get_request_factory() const
{
  TripRequestFactory factory(config);
  return std::make_shared<TripRequestFactory>(factory);
}

void TripApplication::initialize_user_sessions(bool expire_sessions)
{
  if (expire_sessions) {
    SessionManager::get_session_manager()->clear_sessions();
    SessionManager::get_session_manager()->persist_sessions();
    SessionPgDao dao;
    // Actually delete the session records, as persisting them only deletes
    // persisted sessions based on their expiry date
    dao.delete_all_sessions();
  } else {
    SessionManager::get_session_manager()->load_sessions();
  }
}
