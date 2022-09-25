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
#ifndef TILE_PG_DAO_HPP
#define TILE_PG_DAO_HPP

#include "trip_pg_dao.hpp"
#include <string>
#include <vector>

namespace fdsd {
namespace trip {

class TilePgDao : public TripPgDao {
public:
  struct tile_result {
    std::vector<char> tile;
    std::time_t expires;
  };
  long increment_tile_counter();
  void update_tile_count();
  void prune_tile_cache(int max_age);
  void save_tile(int server_id, int z, int x, int y,
                 const std::vector<char> &tile,
                 std::time_t expires);
  std::pair<bool, tile_result> get_tile(int server_id, int z, int x, int y);
};

} // namespace trip
} // namespace fdsd

#endif // TILE_PG_DAO_HPP
