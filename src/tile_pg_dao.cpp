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
#include "tile_pg_dao.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include <chrono>
#include <pqxx/pqxx>

using namespace fdsd::trip;
using namespace fdsd::utils;
using namespace pqxx;

long TilePgDao::increment_tile_counter()
{
  work tx(*connection);
  result r = tx.exec("SELECT nextval('tile_download_seq')");
  tx.commit();
  if (r.empty())
    return 0;
  return r[0][0].as<long>();
}

void TilePgDao::update_tile_count()
{
  work tx(*connection);
  tx.exec("INSERT INTO tile_metric (count) "
          "SELECT currval('tile_download_seq')");
  tx.commit();
}

void TilePgDao::prune_tile_cache(int max_age)
{
  work tx(*connection);
  tx.exec("DELETE FROM tile WHERE expires < now() AND "
          "updated < now()::timestamp::date - " + std::to_string(max_age) +
          " * INTERVAL '1 DAY'");
  tx.commit();
}

void TilePgDao::save_tile(int server_id,
                          int z, int x, int y,
                          const std::vector<char> &tile,
                          std::chrono::system_clock::time_point expires)
{
  // std::cout << "Saving tile"
  //           << server_id << ' ' << z << ' ' << x << ' ' << y << "\n";
  const DateTime expiry_time(expires);
  work tx(*connection);
  std::string sql = "INSERT INTO tile (server_id, z, x, y, expires, image) "
    "VALUES ($1, $2, $3, $4, $5, $6::bytea) "
    "ON CONFLICT (server_id, z, x, y) DO UPDATE "
    "SET server_id=$1, z=$2, x=$3, y=$4, updated=now(), "
    "expires=$5, image=$6::bytea";
#ifdef HAVE_LIBPQXX7
#ifdef ENABLE_LIBPQXX_BINARYSTRING
  // 'binarystring' is deprecated
  const binarystring image(tile.data(), tile.size());
  tx.exec_params(sql,
                 server_id,
                 z, x, y,
                 expiry_time.get_time_as_iso8601_gmt(),
                 image);
#else
  tx.exec_params(sql,
                 server_id,
                 z, x, y,
                 expiry_time.get_time_as_iso8601_gmt(),
                 binary_cast(tile));
#endif // ENABLE_LIBPQXX_BINARYSTRING
#else // !HAVE_LIBPQXX7
  const binarystring image(tile.data(), tile.size());
  tx.exec_params(sql,
                 server_id,
                 z, x, y,
                 expiry_time.get_time_as_iso8601_gmt(),
                 image);
#endif // HAVE_LIBPQXX7
  tx.commit();
  // std::cout << "Saved tile"
  //           << server_id << ' ' << z << ' ' << x << ' ' << y << "\n";
}

std::optional<TilePgDao::tile_result> TilePgDao::get_tile(int server_id,
                                                 int z, int x, int y)
{
  work tx(*connection);
  std::string sql = "SELECT image, updated, expires FROM tile "
    "WHERE server_id=$1 AND z=$2 AND x=$3 AND y=$4";
  result r = tx.exec_params(sql, server_id, z, x, y);
  tx.commit();
  if (r.empty()) {
    return tile_result();
  } else {
    tile_result t;
    DateTime expires(r[0]["expires"].as<std::string>());
    t.tile = std::vector<char>();
    t.expires = expires.time_tp();
    field image = r[0]["image"];
#ifdef HAVE_LIBPQXX7
#ifdef ENABLE_LIBPQXX_BINARYSTRING
    // 'binarystring' is deprecated
    binarystring bs(image);
    for (auto i = bs.cbegin(); i != bs.cend(); i++)
      t.tile.push_back(*i);
#else
    // See https://github.com/jtv/libpqxx/issues/726 and https://github.com/jtv/libpqxx/pull/751
    // and `NEWS` in libpqxx source, section 7.9.0 release
    auto bs = image.as<pqxx::bytes>();
    for (auto i = bs.cbegin(); i != bs.cend(); i++)
      t.tile.push_back((char) *i);
#endif // ENABLE_LIBPQXX_BINARYSTRING
#else // !HAVE_LIBPQXX7
    binarystring bs(image);
    for (auto i = bs.cbegin(); i != bs.cend(); i++)
      t.tile.push_back(*i);
#endif // HAVE_LIBPQXX7
    return t;
  }
}
