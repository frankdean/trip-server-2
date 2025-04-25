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
#include "../config.h"
#include "trip_application.hpp"
#ifdef HAVE_GDAL
#include "elevation_tile.hpp"
#endif
#include "trip_request_factory.hpp"
#include "session_pg_dao.hpp"
#include "../trip-server-common/src/session.hpp"
#include <chrono>
#include <memory>

using namespace fdsd::utils;
using namespace fdsd::web;
using namespace fdsd::trip;

TripApplication::TripApplication(std::string listen_address,
                                 std::string port,
                                 std::string config_filename) :
  Application(
    listen_address,
    port)
{
  config = std::make_shared<TripConfig>(TripConfig(config_filename));
#ifdef HAVE_GDAL
  const std::string elevation_tile_path = config->get_elevation_tile_path();
  if (!elevation_tile_path.empty()) {
    std::string elevation_tile_index_pathname =
      config->get_elevation_tile_index_pathname();
    if (elevation_tile_index_pathname.empty()) {
      elevation_tile_index_pathname = elevation_tile_path + "/.tile-index.json";
    }
    elevation_service.reset(new ElevationService(
        config->get_elevation_tile_path(),
        elevation_tile_index_pathname,
        std::string(),
        config->get_elevation_tile_cache_ms()));
  }
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
  TripRequestFactory factory(config, elevation_service);
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
