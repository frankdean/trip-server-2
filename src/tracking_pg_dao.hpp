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
#ifndef TRACKS_PG_DAO_HPP
#define TRACKS_PG_DAO_HPP

#include "trip_pg_dao.hpp"
#include "geo_utils.hpp"
#include "../trip-server-common/src/dao_helper.hpp"
#include <chrono>
#include <map>
#include <ostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace fdsd {
namespace trip {

class TrackPgDao : public TripPgDao {
public:

  struct location_search_query_params : utils::dao_helper {
    location_search_query_params();
    location_search_query_params(std::string user_id,
                                 const std::map<std::string,
                                 std::string> &params);
    static void to_json(nlohmann::json& j, const location_search_query_params& qp);
    static void from_json(const nlohmann::json& j, location_search_query_params& qp);
    /// this is the integer user_id stored in the session an relates to the
    /// `id` column of `usertable`.
    std::string user_id = "";
    std::string nickname = "";
    std::time_t date_from;
    std::time_t date_to;
    int max_hdop = -1;
    bool notes_only_flag = false;
    dao_helper::result_order order = ascending;
    long page = 1;
    long page_offset = -1;
    long page_size = -1;
    std::string to_string() const;
    inline friend std::ostream& operator<<
        (std::ostream& out, const location_search_query_params& rhs) {
      return out << rhs.to_string();
    }
    std::map<std::string, std::string> query_params();
  private:
    /// Mutex used to lock access to non-threadsafe functions
    static std::mutex mutex;
  };

  /// Holds a result for a tracked location.  Nullable fields use a pair with
  /// a boolean specifying whether the value is not null.  i.e. true == is not
  /// null.
  struct tracked_location : public location {
    virtual ~tracked_location() {}
    std::chrono::system_clock::time_point time_point;
    std::pair<bool, float> hdop;
    std::pair<bool, float> speed;
    std::pair<bool, double> bearing;
    std::pair<bool, int> satellite_count;
    std::pair<bool, std::string> provider;
    std::pair<bool, float> battery;
    std::pair<bool, std::string> note;
    virtual std::string to_string() const override;
    inline friend std::ostream& operator<<
        (std::ostream& out, const tracked_location& rhs) {
      return out << rhs.to_string();
    }
  };

  struct tracked_locations_result {
    tracked_locations_result() : total_count(0), locations() {}
    long total_count;
    std::time_t date_from;
    std::time_t date_to;
    std::vector<tracked_location> locations;
  };

  struct tracked_location_query_params :
    public tracked_location, utils::dao_helper {

    tracked_location_query_params() {}
    tracked_location_query_params(std::string user_id,
                                  const std::map<std::string,
                                  std::string> &params);
    virtual ~tracked_location_query_params() {}
    std::string user_id;
    virtual std::string to_string() const override;
    inline friend std::ostream& operator<<
        (std::ostream& out, const tracked_location_query_params& rhs) {
      return out << rhs.to_string();
    }
  };

  struct nickname_result {
    /// Current user's nickname
    std::string nickname;
    /// List of shared nicknames
    std::vector<std::string> nicknames;
    std::string to_string() const;
    inline friend std::ostream& operator<<
        (std::ostream& out, const nickname_result& rhs) {
      return out << rhs.to_string();
    }
  };

  struct location_share_details {
    std::string shared_to_id;
    std::string shared_by_id;
    std::pair<bool, int> recent_minutes;
    std::pair<bool, int> max_minutes;
    std::pair<bool, bool> active;
  };

  struct track_share {
    std::string nickname;
    std::pair<bool, int> recent_minutes;
    std::pair<bool, int> max_minutes;
    std::pair<bool, bool> active;
  };

  struct location_update_check {
    std::string nickname;
    std::pair<bool, long> min_threshold_id;
    bool update_available;
    location_update_check() :
      nickname(),
      min_threshold_id(),
      update_available(false) {}
  };

  struct date_range {
    std::time_t from;
    std::time_t to;
  };

  struct triplogger_configuration {
    std::string uuid;
    std::string tl_settings;
  };

  /**
   * Fetches a list of nicknames.
   *
   * \param user_id the user id to fetch the information for.
   *
   * \return an object containing the user's nickname and a list of
   * nicknames that are sharing their location data with the specified user.
   */
  nickname_result get_nicknames(std::string user_id);

  /**
   * Gets the tracked locations, either for the current user, or for a shared
   * user, depending on whether nickname is not empty.  The result set is
   * limited to the specified maxium number of results, dropping the oldest
   * results.
   *
   * \param location_search_query_params the search criteria.
   *
   * \param maximum the maximum number of results to return.  Set to -1 for no
   * maximum.
   *
   * \param fill_distance_and_elevation_values true if any empty distance and
   * elevation values in the locations points are to be calculated and looked
   * up, respectively.
   *
   * \return a tracked_locations_result object
   */
  tracked_locations_result get_tracked_locations(
      const location_search_query_params& location_search,
      int maximum,
      bool fill_distance_and_elevation_values = false) const;

  /// \return the user_id associated with the passed UUID
  std::string get_user_id_by_uuid(std::string uuid);
  /// \return the uuid used for logging tracked locations for the given user_id
  std::string get_logging_uuid_by_user_id(std::string user_id);
  std::string get_user_id_by_nickname(std::string nickname);
  /// Saves the passed logging UUID for the specified user
  void save_logging_uuid(std::string user_id, std::string logging_uuid);
  /// Saves the passed tracked location.
  void save_tracked_location(
      const TrackPgDao::tracked_location_query_params& qp);
  /// Saves the passed location_share_details
  void save(const location_share_details& share);
  triplogger_configuration get_triplogger_configuration(std::string user_id);
  void save(std::string user_id,
            const triplogger_configuration & tl_config);
  long get_track_sharing_count_by_user_id(std::string user_id);
  std::vector<track_share> get_track_sharing_by_user_id(
      std::string user_id,
      std::uint32_t offset,
      int limit);
  void delete_track_shares(std::string user_id,
                           std::vector<std::string> nicknames);
  void activate_track_shares(std::string user_id,
                             std::vector<std::string> nicknames,
                             bool activate = true);
  void deactivate_track_shares(std::string user_id,
                               std::vector<std::string> nicknames) {
    activate_track_shares(user_id, nicknames, false);
  }
  std::pair<bool, location_share_details>
      get_tracked_location_share_details_by_sharer(
          std::string shared_to_nickname,
          std::string shared_by_user_id) const;
  bool check_new_locations_available(
      std::string user_id,
      std::string nickname,
      long min_id_threshold);
  void check_new_locations_available(
      std::string user_id,
      std::chrono::system_clock::time_point since,
      std::vector<location_update_check> &criteria);
  std::string get_nickname_for_user_id(std::string user_id);
private:
  std::string get_nickname_for_user_id(pqxx::work &tx, std::string user_id);
  date_range constrain_shared_location_dates(
      std::string shared_by_id,
      std::time_t date_from,
      std::time_t date_to,
      std::pair<bool, int> max_minutes,
      std::pair<bool, int> recent_minutes) const;
  std::pair<bool, std::time_t>
      get_most_recent_location_time(std::string shared_by_id) const;
  tracked_locations_result get_tracked_locations_for_user(
      const location_search_query_params& location_search, int maximum) const;
  tracked_locations_result get_shared_tracked_locations(
      const location_search_query_params& qp, int maximum) const;
  void append_location_query_where_clause(
      std::ostringstream& os,
      const pqxx::work& tx,
      const location_search_query_params& qp) const;
  std::pair<bool, location_share_details>
      get_tracked_location_share_details_by_sharee(
          std::string shared_by_nickname,
          std::string shared_to_user_id) const;
};

/// Allows Argument-depenedent lookup for the nlohmann/json library to find this method
void to_json(nlohmann::json& j, const TrackPgDao::location_search_query_params& qp);

/// Allows Argument-depenedent lookup for the nlohmann/json library to find this method
void from_json(const nlohmann::json& j, TrackPgDao::location_search_query_params& qp);

} // namespace trip
} // namespace fdsd

#endif // TRACKS_PG_DAO_HPP
