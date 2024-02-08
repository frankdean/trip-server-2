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
#ifndef ITINERARY_HANDLER_HPP
#define ITINERARY_HANDLER_HPP

#include "trip_request_handler.hpp"
#include "itinerary_pg_dao.hpp"
#include "tracking_pg_dao.hpp"
#include "trip_config.hpp"
#include <nlohmann/json.hpp>

namespace fdsd {
namespace web {
  class HTTPServerRequest;
  class HTTPServerResponse;
  class Pagination;
}
namespace trip {

class ElevationService;

class ItineraryHandler : public TripAuthenticatedRequestHandler {

  std::shared_ptr<ElevationService> elevation_service;
  ItineraryPgDao itinerary_dao;

public:

  struct paste_features : public ItineraryPgDao::selected_feature_ids {
    long itinerary_id;
    paste_features()  : ItineraryPgDao::selected_feature_ids() {}
    paste_features(long itinerary_id,
                   ItineraryPgDao::selected_feature_ids features)
      : ItineraryPgDao::selected_feature_ids(features),
        itinerary_id(itinerary_id) {}

    static void to_json(nlohmann::json& j, const paste_features& ids);
    static void from_json(const nlohmann::json& j, paste_features& ids);
  };

private:
  bool feature_copy_success;
  bool read_only;
  bool show_raw_markdown;
  bool max_track_paste_exceeded;
  long itinerary_id;
  enum active_tab_type {
    itinerary_tab,
    features_tab
  };
  active_tab_type active_tab;
  std::pair<bool, TrackPgDao::location_search_query_params>
      location_history_paste_params;
  std::pair<bool, paste_features>
      selected_features_paste_params;

  std::pair<bool, TrackPgDao::location_search_query_params>
      get_location_history_paste_params();

  std::pair<bool, paste_features>
      get_selected_features_paste_params();

  void paste_items();

  void paste_locations();

  void paste_itinerary_features();

  void append_heading_content(
      web::HTTPServerResponse& response,
      const ItineraryPgDao::itinerary& itinerary);
  void append_itinerary_content(
      web::HTTPServerResponse& response,
      const ItineraryPgDao::itinerary& itinerary);
  void append_path(
      std::ostream &os,
      const ItineraryPgDao::path_summary &path,
      std::string path_type,
      bool estimate_time);
  void append_waypoint(
      std::ostream &os,
      const ItineraryPgDao::waypoint_summary &waypoint);
  void append_features_content(
      web::HTTPServerResponse& response,
      const ItineraryPgDao::itinerary& itinerary);
  void build_form(web::HTTPServerResponse& response,
                  const ItineraryPgDao::itinerary& itinerary);
  double get_kms_per_hour() {
    const auto trip_config = std::static_pointer_cast<TripConfig>(config);
    return trip_config->get_default_average_kmh_hiking_speed();
  }
  double calculate_scarfs_equivalence_kilometers(
      double distance, double ascent) {
    const double retval = (distance + ascent * 7.92 / 1000) / get_kms_per_hour();
    // std::cout << "Calculated scarf's as " << retval << " hours for "
    //           << distance << " kms distance and " << ascent << "m ascent\n";
    return retval;
  }
  void convertTracksToRoutes(const web::HTTPServerRequest& request);
  void auto_color_paths(const web::HTTPServerRequest& request);
  void join_routes(const web::HTTPServerRequest &request,
                   web::HTTPServerResponse& response,
                   const std::vector<long> &route_ids);
  void join_tracks(const web::HTTPServerRequest &request,
                   web::HTTPServerResponse& response,
                   const std::vector<long> &track_ids);
protected:
  virtual void do_preview_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const web::HTTPServerRequest& request,
      web::HTTPServerResponse& response) override;
public:
  ItineraryHandler(std::shared_ptr<TripConfig> config,
                   std::shared_ptr<ElevationService> elevation_service) :
    TripAuthenticatedRequestHandler(config),
    elevation_service(elevation_service),
    itinerary_dao(),
    feature_copy_success(false),
    read_only(true),
    show_raw_markdown(false),
    max_track_paste_exceeded(false),
    itinerary_id(),
    active_tab(itinerary_tab),
    location_history_paste_params() {}
  virtual ~ItineraryHandler() {}
  virtual std::string get_handler_name() const override {
    return "ItineraryHandler";
  }
  virtual bool can_handle(
      const web::HTTPServerRequest& request) const override {
    return compare_request_regex(request.uri, "/itinerary($|\\?.*)");
  }
  virtual std::unique_ptr<web::BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ItineraryHandler>(
        new ItineraryHandler(config, elevation_service));
  }
  static ItineraryPgDao::selected_feature_ids get_selected_feature_ids(
      const web::HTTPServerRequest& request);
};

} // namespace trip
} // namespace fdsd

#endif // ITINERARY_HANDLER_HPP
