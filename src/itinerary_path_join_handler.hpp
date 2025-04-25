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
#ifndef ITINERARY_PATH_JOIN_HANDLER_HPP
#define ITINERARY_PATH_JOIN_HANDLER_HPP

#include "trip_request_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include <optional>
#include <string>
#include <vector>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class ItineraryPathJoinHandler : public BaseMapHandler {
protected:
  long itinerary_id;
  std::vector<long> path_ids;
  /// Action parameter
  std::string action;
  std::optional<std::string> joined_path_name;
  std::optional<std::string> joined_path_color_key;
  /// The available colors for selection
  std::vector<std::pair<std::string, std::string>> colors;
  /// Holds the path ID from an `up` button submit
  std::map<long, std::string> up_action_map;
  /// Holds the path ID from an `down` button submit
  std::map<long, std::string> down_action_map;
  std::map<long, std::string> posted_paths_map;
  /// List of the selected paths read from the DB
  std::vector<ItineraryPgDao::path_summary> paths;
  void build_form(std::ostream &os,
                  const std::vector<ItineraryPgDao::path_summary> paths);
  virtual std::vector<ItineraryPgDao::path_summary>
      get_paths(ItineraryPgDao &dao) = 0;
  virtual void join_paths(ItineraryPgDao &dao, std::vector<long> ids) = 0;
  virtual ItineraryPgDao::selected_feature_ids get_selected_features() = 0;
  virtual void do_preview_request(const web::HTTPServerRequest& request,
                                  web::HTTPServerResponse& response) override;
  virtual void append_pre_body_end(std::ostream& os) const override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  ItineraryPathJoinHandler(std::shared_ptr<TripConfig> config) :
    BaseMapHandler(config),
    itinerary_id(),
    path_ids(),
    action(),
    joined_path_name(),
    joined_path_color_key(),
    colors(),
    up_action_map(),
    down_action_map(),
    posted_paths_map(),
    paths() {}
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_PATH_JOIN_HANDLER_HPP
