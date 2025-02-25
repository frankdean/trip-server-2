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
#include "trip_config.hpp"
#include "../trip-server-common/src/uuid.hpp"
#include <iostream>
#include <syslog.h>

using namespace fdsd::trip;
using namespace fdsd::utils;

TripConfig::TripConfig(std::string filename) :
  root_directory(),
  user_guide_path("./static/doc/trip-user-guide.html/index.html"),
  application_prefix_url(),
  db_connect_string("postgresql://%2Fvar%2Frun%2Fpostgresql/trip"),
  worker_count(20),
  pg_pool_size(24),
  maximum_request_size(1024 * 1024 * 12),
  /// Default session timeout in minutes
  session_timeout(60),
  providers(),
  tile_cache_max_age(),
  tile_count_frequency(),
  gpx_pretty(false),
  gpx_indent(2),
  default_average_kmh_hiking_speed(4),
  elevation_tile_cache_ms(),
  elevation_tile_path(),
  elevation_tile_index_pathname(),
  proj_search_path(),
  maximum_location_tracking_points(10000),
  default_triplogger_configuration()
{
  try {
    if (filename.empty())
      filename = SYSCONFDIR "/trip-server.yaml";
    // std::cout << "Reading config from \"" << filename << "\"\n";
    auto yaml = YAML::LoadFile(filename);
    if (auto db = yaml["db"]) {
      db_connect_string = db["uri"].as<std::string>();
    }
    if (auto app = yaml["app"]) {
      if (app["worker_count"])
        worker_count = app["worker_count"].as<int>();
      if (app["pg_pool_size"])
        pg_pool_size = app["pg_pool_size"].as<int>();
      if (auto gpx = app["gpx"]) {
        if (gpx["pretty"])
          gpx_pretty = gpx["pretty"].as<bool>();
        if (gpx["indent"])
          gpx_indent = gpx["indent"].as<int>();
      }
      if (app["maximum_location_tracking_points"])
        maximum_location_tracking_points =
          app["maximum_location_tracking_points"].as<int>();
      if (app["maxFileUploadSize"])
        maximum_request_size = app["maxFileUploadSize"].as<long>();
      if (app["session_timeout"])
        session_timeout = app["session_timeout"].as<int>();
      if (app["averageFlatSpeedKph"])
        default_average_kmh_hiking_speed =
          app["averageFlatSpeedKph"].as<double>();

      std::string prefix_url;
      if (app["prefix_url"])
        prefix_url = app["prefix_url"].as<std::string>();
      // Any configuration for the application URL takes precedence over the
      // default passed to the constructor.
      if (prefix_url.empty()) {
        prefix_url = "/trip/app";
      } else if (prefix_url.length() > 1 &&
                 prefix_url.substr(
                     prefix_url.length() -1, 1) == "/") {
        prefix_url.erase(prefix_url.length() -1, 1);
      }
      this->application_prefix_url = prefix_url;
      if (app["user_guide"])
        user_guide_path = app["user_guide"].as<std::string>();
    }
    if (auto reporting = yaml["reporting"]) {
      if (auto metrics = reporting["metrics"])
        if (auto tile_metrics = metrics["tile"])
          if (auto tile_metrics_count = tile_metrics["count"])
            tile_count_frequency = tile_metrics_count["frequency"].as<int>();
    }
    // process tile provider configurations
    if (auto t = yaml["tile"]) {
      if (auto c = t["cache"]) {
        if (c["maxAge"])
          tile_cache_max_age = c["maxAge"].as<int>();
      }
      if (const auto &providers_config = t["providers"]) {
        for (const auto cp : providers_config) {
          tile_provider p;
          if (cp["cache"])
            p.cache = cp["cache"].as<bool>();
          if (cp["prune"])
            p.prune = cp["prune"].as<bool>();
          if (cp["help"])
            p.help = cp["help"].as<std::string>();
          if (cp["userAgentInfo"])
            p.user_agent_info = cp["userAgentInfo"].as<std::string>();
          if (cp["referrerInfo"]) {
            p.referrer_info = cp["referrerInfo"].as<std::string>();
          } else if (cp["refererInfo"]) {
            // refererInfo is a miss-spelling carried over from the earlier version
            p.referrer_info = cp["refererInfo"].as<std::string>();
          }
          if (auto options = cp["options"]) {
            if (options["protocol"])
              p.protocol = options["protocol"].as<std::string>();
            if (options["host"])
              p.host = options["host"].as<std::string>();
            if (options["port"])
              p.port = options["port"].as<std::string>();
            if (options["path"])
              p.path = options["path"].as<std::string>();
            if (options["method"])
              p.method = options["method"].as<std::string>();
          }
          if (auto map_layer = cp["mapLayer"]) {
            if (map_layer["name"])
              p.name = map_layer["name"].as<std::string>();
            if (map_layer["type"])
              p.type = map_layer["type"].as<std::string>();
            if (map_layer["minZoom"])
              p.min_zoom = map_layer["minZoom"].as<int>();
            if (map_layer["maxZoom"])
              p.max_zoom = map_layer["maxZoom"].as<int>();
            if (auto tile_attributions = map_layer["tileAttributions"]) {
              p.tile_attributions_html = "";
              for (const auto &a : tile_attributions) {
                std::string link;
                std::string text;
                std::string title;
                if (a["link"])
                  link = a["link"].as<std::string>();
                if (a["text"])
                  text = a["text"].as<std::string>();
                if (link.empty()) {
                  p.tile_attributions_html.append(text);
                } else if (!text.empty()) {
                  if (a["title"])
                    title = a["title"].as<std::string>();
                  p.tile_attributions_html.append("<a href=\"").append(link)
                    .append("\" target=\"_blank\"");
                  if (!title.empty()) {
                    p.tile_attributions_html.append(" title=\"").append(title)
                      .append("\"");
                  }
                  p.tile_attributions_html.append(">").append(text).
                    append("</a>");
                }
              }
            }
          }
          providers.push_back(p);
        }
      }
      // std::cout << "Tile configuration max age " << tile_cache_max_age << "\n";
      // for (const auto &p : providers) {
      //   std::cout << p << '\n';
      // }
    }
    if (auto tl = yaml["tripLogger"]) {
      default_triplogger_configuration = tl["defaultConfiguration"];
    }
    if (auto elevation_tiles = yaml["elevation"]) {
      if (elevation_tiles["tileCacheMs"])
        elevation_tile_cache_ms = elevation_tiles["tileCacheMs"].as<int>();
      if (elevation_tiles["datasetDir"])
        elevation_tile_path = elevation_tiles["datasetDir"].as<std::string>();
      if (elevation_tiles["datasetDirIndex"])
        elevation_tile_index_pathname =
          elevation_tiles["datasetDirIndex"].as<std::string>();
    }
  } catch (const YAML::BadFile& e) {
    syslog(LOG_ERR, "Failure reading \"%s\": %s",
           filename.c_str(),
           e.what());
  } catch (const YAML::ParserException& e) {
    syslog(LOG_ERR, "Failure parsing \"%s\": %s",
           filename.c_str(),
           e.what());
  }
}

YAML::Node TripConfig::create_default_triplogger_configuration()
{
  const std::string new_profile_uuid = UUID::generate_uuid();
  YAML::Node retval = default_triplogger_configuration["defaultSettings"];
  retval["currentSettingUUID"] = new_profile_uuid;
  if (auto profile = default_triplogger_configuration["defaultProfile"]) {
    profile["uuid"] = new_profile_uuid;
    retval["settingProfiles"].push_back(profile);
  }
  return retval;
}

std::string tile_provider::to_string() const
{
  std::ostringstream os;
  os << "cache: " << (cache ? "true" : "false") << ", "
     << "prune: " << prune << ", "
     << "help: \"" << help << "\", "
     << "user_agent_info: \"" << user_agent_info << "\", "
     << "referrer_info: \"" << referrer_info << "\", "
     << "options: {" << HttpOptions::to_string() << "}, "
     << "name: \"" << name << "\", "
     << "type: \"" << type << "\", "
     << "min_zoom: " << min_zoom << ", "
     << "max_zoom: " << max_zoom << "', "
     << "tile_attributions_html: \"" << tile_attributions_html << '"';
  return os.str();
}
