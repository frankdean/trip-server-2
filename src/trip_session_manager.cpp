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
#include "../config.h"
#include "session_pg_dao.hpp"
#include "trip_config.hpp"
#include "trip_session_manager.hpp"
#include "trip_pg_dao.hpp"

using namespace fdsd::web;
using namespace fdsd::trip;

const std::string TripSessionManager::session_id_cookie_name = "TRIP_SESSION_ID";

void TripSessionManager::create_session_table(bool overwrite)
{
  SessionPgDao dao;
  dao.create_session_table(overwrite);
}

void TripSessionManager::session_updated(std::string session_id,
                                         const Session& session) const
{
  SessionPgDao dao;
  dao.save_session(session_id, session);
}

void TripSessionManager::persist_sessions()
{
  expire_sessions();
  std::lock_guard<std::mutex> lock(session_mutex);
  SessionPgDao dao;
  dao.save_sessions(sessions);
}

void TripSessionManager::load_sessions()
{
  std::lock_guard<std::mutex> lock(session_mutex);
  SessionPgDao dao;
  dao.load_sessions(sessions);
  session_mutex.unlock();
  expire_sessions();
}

int TripSessionManager::get_max_session_minutes()
{
  return config->get_session_timeout();
}

void TripSessionManager::persist_invalidated_session(const std::string session_id)
{
  // Don't use mutex as the mutex is locked when this method is called
  // std::lock_guard<std::mutex> lock(session_mutex);
  SessionPgDao dao;
  dao.invalidate_session(session_id);
}
