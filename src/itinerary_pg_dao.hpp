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
#ifndef ITINERARY_PG_DAO_HPP
#define ITINERARY_PG_DAO_HPP

#include "trip_pg_dao.hpp"
#include "geo_utils.hpp"
#include <chrono>
#include <string>
#include <vector>


namespace fdsd {
namespace trip {

class ItineraryPgDao : public TripPgDao {

public:
  struct path_base {
    long id;
    std::pair<bool, double> distance;
    std::pair<bool, double> ascent;
    std::pair<bool, double> descent;
    std::pair<bool, double> lowest;
    std::pair<bool, double> highest;
  };
  struct path_summary : public path_base {
    std::pair<bool, std::string> name;
    std::pair<bool, std::string> color;
  };
  struct route_point : public location {
    std::pair<bool, std::string> name;
    std::pair<bool, std::string> comment;
    std::pair<bool, std::string> description;
    std::pair<bool, std::string> symbol;
  };
  struct route : public path_summary {
    std::vector<std::unique_ptr<route_point>> points;
  };
  struct track_point : public location {
    std::pair<bool, std::chrono::system_clock::time_point> time;
    std::pair<bool, float> hdop;
  };
  struct track_segment : public path_base {
    std::vector<std::unique_ptr<track_point>> points;
  };
  struct track : public path_summary {
    std::vector<std::unique_ptr<track_segment>> segments;
  };
  struct waypoint_base {
    std::pair<bool, std::string> name;
    std::pair<bool, std::string> comment;
    std::pair<bool, std::string> symbol;
    std::pair<bool, std::string> type;
  };
  struct waypoint_summary : public waypoint_base {
    long id;
  };
  struct waypoint : public location, public waypoint_base {
    std::pair<bool, std::chrono::system_clock::time_point> time;
    std::pair<bool, std::string> description;
    std::pair<bool, std::string> color;
    std::pair<bool, std::string> type;
    std::pair<bool, long> avg_samples;
  };
  struct itinerary_base {
    std::pair<bool, long> id;
    std::pair<bool, std::chrono::system_clock::time_point> start;
    std::pair<bool, std::chrono::system_clock::time_point> finish;
    std::string title;
  };
  struct itinerary_summary : public itinerary_base {
    std::pair<bool, std::string> owner_nickname;
    std::pair<bool, bool> shared;
  };
  struct itinerary_description : public itinerary_summary {
    std::pair<bool, std::string> description;
  };
  struct itinerary_detail : public itinerary_description {
    std::pair<bool, std::string> shared_to_nickname;
  };
  struct itinerary : public itinerary_detail {
    std::pair<bool, std::string> description;
    std::vector<path_summary> routes;
    std::vector<path_summary> tracks;
    std::vector<waypoint_summary> waypoints;
  };
  long get_itineraries_count(
      std::string user_id);
  std::vector<itinerary_summary> get_itineraries(
      std::string user_id,
      std::uint32_t offset,
      int limit);
  std::pair<bool, ItineraryPgDao::itinerary>
      get_itinerary_details(
          std::string user_id, long itinerary_id);
  std::pair<bool, ItineraryPgDao::itinerary_description>
      get_itinerary_description(
          std::string user_id, long itinerary_id);
  void save_itinerary(
      std::string user_id,
      ItineraryPgDao::itinerary_description itinerary);
  void delete_itinerary(
      std::string user_id,
      long itinerary_id);
  std::vector<std::unique_ptr<route>>
      get_routes(std::string user_id,
                 long itinerary_id,
                 std::vector<long> route_ids);
  std::vector<std::unique_ptr<ItineraryPgDao::track>>
      get_tracks(std::string user_id,
                 long itinerary_id,
                 std::vector<long> ids);
  std::vector<std::unique_ptr<ItineraryPgDao::waypoint>>
      get_waypoints(std::string user_id,
                    long itinerary_id,
                    std::vector<long> ids);
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_PG_DAO_HPP
