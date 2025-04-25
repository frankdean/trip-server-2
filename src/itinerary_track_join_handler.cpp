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
#include "itinerary_track_join_handler.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/uri_utils.hpp"
#include <boost/locale.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>

using namespace boost::locale;
using namespace fdsd::trip;
using namespace fdsd::web;
using namespace fdsd::utils;
using json = nlohmann::basic_json<nlohmann::ordered_map>;

void ItineraryTrackJoinHandler::do_preview_request(
    const web::HTTPServerRequest& request,
    web::HTTPServerResponse& response)
{
  ItineraryPathJoinHandler::do_preview_request(request, response);
  // Title of the page for joining itinerary tracks
  set_page_title(translate("Join Itinerary Tracks"));
}

std::vector<ItineraryPgDao::path_summary>
    ItineraryTrackJoinHandler::get_paths(ItineraryPgDao &dao)
{
  return dao.get_track_summaries(get_user_id(), itinerary_id, path_ids);
}

void ItineraryTrackJoinHandler::join_paths(ItineraryPgDao &dao,
                                           std::vector<long> ids)
{
  const auto tracks = dao.get_tracks(get_user_id(), itinerary_id, ids);

  // Re-order the paths read from the database to match the user specified order
  // for joining
  std::map<long, ItineraryPgDao::track> temp_map;
  for (const auto &r : tracks)
    if (r.id.has_value())
      temp_map[r.id.value()] = r;
  std::vector<ItineraryPgDao::track> ordered_tracks;
  for (const auto &p : posted_paths_map)
    ordered_tracks.push_back(temp_map.at(std::stol(p.second)));

  ItineraryPgDao::track joined_track;
  if (joined_path_name) {
    joined_track.name = joined_path_name;
  }
  joined_track.color_key = joined_path_color_key;

  for (const auto &r : ordered_tracks)
    for (const auto &seg : r.segments)
      joined_track.segments.push_back(seg);

  joined_track.calculate_statistics();
  dao.save(get_user_id(), itinerary_id, joined_track);
}

ItineraryPgDao::selected_feature_ids
    ItineraryTrackJoinHandler::get_selected_features()
{
  ItineraryPgDao::selected_feature_ids features;
  features.tracks = path_ids;
  return features;
}
