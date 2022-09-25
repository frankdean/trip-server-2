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
#ifndef TRIP_CONFIG_HPP
#define TRIP_CONFIG_HPP

#include "../trip-server-common/src/http_client.hpp"
#include <string>

namespace fdsd {
namespace trip {

struct tile_provider : public fdsd::web::HttpOptions {
  bool cache = true;
  bool prune = true;
  std::string help;
  std::string user_agent_info;
  std::string referrer_info;
  std::string name;
  std::string type;
  int min_zoom;
  int max_zoom;
  std::string tile_attributions_html;
  virtual std::string to_string() const override;
  inline friend std::ostream& operator<<
      (std::ostream& out, const tile_provider& rhs) {
    return out << rhs.to_string();
  }
};

class TripConfig {
  std::string root_directory;
  std::string application_prefix_url;
  std::string db_connect_string;
  int worker_count;
  int pg_pool_size;
  std::vector<tile_provider> providers;
  int tile_cache_max_age;
  int tile_count_frequency;
public:
  TripConfig(std::string filename);
  std::string get_root_directory() const {
    return root_directory;
  }
  void set_root_directory(std::string directory) {
    root_directory = directory;
  }
  std::string get_application_prefix_url() const {
    return application_prefix_url;
  }
  std::string get_db_connect_string() const {
    return db_connect_string;
  }
  int get_worker_count() const {
    return worker_count;
  }
  int get_pg_pool_size() const {
    return pg_pool_size;
  }
  int get_tile_cache_max_age() const {
    return tile_cache_max_age;
  }
  std::vector<tile_provider> get_providers() const {
    return providers;
  }
  int get_tile_count_frequency() const {
    return tile_count_frequency;
  }
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_CONFIG_HPP
