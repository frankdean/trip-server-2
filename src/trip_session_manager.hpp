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
#ifndef TRIP_SESSION_HPP
#define TRIP_SESSION_HPP

namespace fdsd {

namespace web {
  class SessionManager;
  class Session;
}

namespace trip {

class TripConfig;

class TripSessionManager : public fdsd::web::SessionManager
{
protected:
  std::shared_ptr<TripConfig> config;
  virtual void session_updated(std::string session_id,
                               const fdsd::web::Session& session) const override;
  virtual void persist_invalidated_session(const std::string session_id) override;
  void create_session_table(bool overwrite = false);
public:
  TripSessionManager(std::shared_ptr<TripConfig> config) : SessionManager(), config(config) {}
  virtual ~TripSessionManager() {}
  static const std::string session_id_cookie_name;
  virtual void persist_sessions() override;
  virtual void load_sessions() override;
  virtual int get_max_session_minutes() override;
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_SESSION_HPP
