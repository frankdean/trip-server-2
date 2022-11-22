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
#ifndef TRIP_APPLICATION_HPP
#define TRIP_APPLICATION_HPP

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include "../trip-server-common/src/application.hpp"

namespace fdsd
{
namespace trip
{

class TripConfig;

class TripApplication : public fdsd::web::Application {
private:
  std::shared_ptr<TripConfig>config;
  std::string config_filename;
protected:
  virtual std::shared_ptr<web::HTTPRequestFactory> get_request_factory() const override;
public:
  TripApplication(std::string listen_address,
                  std::string port,
                  std::string config_filename,
                  std::string locale = "");
  virtual ~TripApplication();
  std::string get_config_filename() const override;
  std::string get_db_connect_string() const;
  int get_worker_count() const;
  int get_pg_pool_size() const;
  std::string get_application_prefix_url() const;
  void set_root_directory(std::string directory);
  void initialize_user_sessions(bool expire_sessions);
  std::shared_ptr<TripConfig> get_config() {
    return config;
  }
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_APPLICATION_HPP
