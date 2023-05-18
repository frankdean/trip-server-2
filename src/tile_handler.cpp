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
#include "../config.h"
#include "tile_handler.hpp"
#include "trip_config.hpp"
#include "../trip-server-common/src/date_utils.hpp"
#include "../trip-server-common/src/http_client.hpp"
#include "../trip-server-common/src/http_response.hpp"
#include "../trip-server-common/src/uri_utils.hpp"
#ifdef HAVE_CAIRO
#include <cairommconfig.h>
#include <cairomm/context.h>
#endif
#include <algorithm>
#include <functional>
#include <iostream>

using namespace fdsd::trip;
using namespace fdsd::utils;
using namespace fdsd::web;

const std::regex TileHandler::tile_url_re = std::regex(".*/tile/(-?\\d+)/(\\d+)/(\\d+)/(\\d+).png");

TileHandler::TileHandler(std::shared_ptr<TripConfig> config)
  : BaseRestHandler(config)
{
}

void TileHandler::set_content_headers(HTTPServerResponse& response) const
{
  response.set_header("Content-Length", std::to_string(response.content.str().length()));
  response.set_header("Content-Type", get_mime_type("png"));
  response.set_header("Cache-Control", "no-cache");
}

bool TileHandler::can_handle(
    const HTTPServerRequest& request) const
{
  const std::string wanted_url = get_uri_prefix() + "/tile";
  return !request.uri.empty() &&
    request.uri.compare(0, wanted_url.length(), wanted_url) == 0;
}

tile_provider TileHandler::get_provider(int provider_index) const
{
  auto providers = config->get_providers();
  if (provider_index < 0 || provider_index >= providers.size()) {
    throw std::invalid_argument("Provider index out of range");
  }
  return providers[provider_index];
}

TilePgDao::tile_result TileHandler::fetch_remote_tile(
    int provider_index,
    int z,
    int x,
    int y) const
{
  TilePgDao::tile_result retval;
  auto provider = get_provider(provider_index);
  std::string path = provider.path;
  // std::cout << "Provider: " << provider << '\n';
  // std::cout << "Path before: \"" << path << "\"\n";
  path.replace(path.find("{z}"), 3, std::to_string(z));
  path.replace(path.find("{x}"), 3, std::to_string(x));
  path.replace(path.find("{y}"), 3, std::to_string(y));
  provider.path = UriUtils::uri_encode_rfc_1738(path);
  // std::cout << "Path after: \"" << path << "\"\n";
  provider.add_header("Host", provider.host);
  provider.add_header("Referer", "https://www.fdsd.co.uk/trip/about.html");
  provider.add_header("User-Agent",
                      UriUtils::uri_encode_rfc_1738(
                          PACKAGE "/" PACKAGE_VERSION
                          " (mailto:support@fdsd.co.uk)"));
  provider.add_header("Accept", "*/*");
  HttpClient client(provider);
  // std::cout << "Options: " << provider.HttpOptions::to_string() << '\n';
  client.perform_request();
  if (client.status_code != 200) {
    std::ostringstream msg;
    msg << "Failure retrieving tile from \""
        << provider.name
        << "\" z: " << z
        << " x: " << x
        << " y: " << y
        << " status code: " << client.status_code << '\n';
    throw tile_not_found_exception(msg.str());
  }
  TilePgDao dao;
  auto tile_count = dao.increment_tile_counter();
  if (tile_count % std::static_pointer_cast<TripConfig>(config)->get_tile_count_frequency() == 0) {
    dao.update_tile_count();
    if (provider.prune == true)
      dao.prune_tile_cache(std::static_pointer_cast<TripConfig>(config)->get_tile_cache_max_age());
  }

  std::string expires_str = client.get_header("Expires");
  // std::cout << "Header states tile expires: \"" << expires_str << "\"\n";
  std::tm tm {};
  std::istringstream is(expires_str);
  is.imbue(std::locale("C"));
  is >> std::get_time(&tm, "%a, %d %b %Y %T %Z");
  auto expires_tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
  // Use the expires value from the source tile provider
  retval.expires = expires_tp;
  if (provider.cache == true) {
    auto now = std::chrono::system_clock::now();
    // Set minimum, increased if the expires header is not specified
    auto min = expires_str.empty() ? std::chrono::hours(4) :
      std::chrono::minutes(10);
    auto time_tp = std::max(expires_tp, now + min);
    dao.save_tile(provider_index, z, x, y, client.body, time_tp);
  }
  retval.tile = client.body;
  return retval;
}

#ifdef HAVE_CAIRO
TilePgDao::tile_result TileHandler::create_test_tile(int z, int x, int y)
{
  const double height = 256;
  const double width = 256;
  const double text_y_margin_factor = 1.5;
  const double text_x_margin_factor = 1.1;
  // surface and ctx are reference-counting pointers that will delete the object
  // when the reference goes out of scope
  auto surface =
    Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
  auto ctx = Cairo::Context::create(surface);
  ctx->save();
  // grey background
  ctx->set_source_rgb(0.8353, 0.8353, 0.8353);
  ctx->paint();

  std::vector<double> dashes(1, 3);
  ctx->set_dash(dashes, 0);
  ctx->set_source_rgb(0.0, 0.501961, 0.0);
  ctx->rectangle(0.0, 0.0, surface->get_width(), surface->get_height());
  ctx->stroke();
  ctx->restore();

  const double font_size = 16.0;
  std::ostringstream label;
  label << "x=" << x << " y=" << y << " z=" << z;
  Cairo::TextExtents text_extents;
  ctx->set_font_size(font_size);
  ctx->get_text_extents(label.str(), text_extents);
  const double text_origin_y = (height + text_extents.height + text_extents.y_bearing / 2.0) / 2.0;
  const double text_origin_x = (width - text_extents.width) / 2.0;
  const double box_height = text_extents.height * text_y_margin_factor - text_extents.y_bearing;
  const double box_width = text_extents.width * text_x_margin_factor;

  ctx->set_source_rgb(1.0, 1.0, 1.0);
  double x1 = (width - box_width) / 2.0;
  double y1 = (height - box_height) / 2.0;
  ctx->rectangle(x1,
                 y1,
                 box_width,
                 box_height);
  ctx->fill();

  ctx->move_to(text_origin_x, text_origin_y);
  ctx->set_source_rgb(0.0, 0.0, 0.0);
  ctx->select_font_face("sans-serif", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
  ctx->show_text(label.str());

  // debug vertical center
  // ctx->save();
  // ctx->set_line_width(1.0);
  // ctx->set_source_rgb(1.0, 0.0, 0.0);
  // ctx->move_to(0, height / 2.0);
  // ctx->line_to(width, height / 2.0);
  // ctx->stroke();
  // ctx->restore();

  TilePgDao::tile_result r;
  r.expires = std::chrono::system_clock::now() + std::chrono::hours(24);
  surface->write_to_png_stream(
      [&r] (const unsigned char* data, unsigned int length) {
        for (int i = 0; i < length; i++)
          r.tile.push_back(data[i]);
        return CAIRO_STATUS_SUCCESS;
      });
  return r;
}

#endif

TilePgDao::tile_result TileHandler::find_tile(int provider_index,
                                         int z,
                                         int x,
                                         int y)
{
#ifdef HAVE_CAIRO
  if (provider_index < 0)
    return create_test_tile(z, x, y);
#endif
  const auto provider = get_provider(provider_index);
  if (provider.cache == true) {
    TilePgDao dao;
    const auto tile_result = dao.get_tile(provider_index, z, x, y);
    if (tile_result.first) {
      DateTime dt(tile_result.second.expires);
      // std::cout << "Read tile from cache.  Expires: " << dt << '\n';
      // Treat as not expired when max_age is greater than zero and this tile
      // is still within that period
      const int max_age = std::static_pointer_cast<TripConfig>(config)->
        get_tile_cache_max_age();
      const auto keep_tiles_until = max_age > 0
        ?
        std::chrono::system_clock::now() + std::chrono::hours(max_age * 24)
        :
        std::chrono::system_clock::now();

      if (tile_result.second.expires < keep_tiles_until) {
        try {
          return fetch_remote_tile(provider_index, z, x, y);
        } catch (const tile_not_found_exception& ex) {
          return tile_result.second;
        }
      } else {
        return tile_result.second;
      }
    }
  }
  return fetch_remote_tile(provider_index, z, x, y);
}

void TileHandler::handle_authenticated_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  // std::cout << "tile handler\n";
  std::smatch m;
  if (std::regex_match(request.uri, m, tile_url_re) && m.size() > 4) {
    try {
      int provider_index = std::stoi(m[1]);
      int z = std::stoi(m[2]);
      int x = std::stoi(m[3]);
      int y = std::stoi(m[4]);
      auto result = find_tile(provider_index, z, x, y);
      std::for_each(result.tile.begin(), result.tile.end(),
                    [&response](const char c) {
                      response.content << c;
                    });
      // The date may be in the past - fine - it's expired!
      DateTime expires(result.expires);
      response.set_header("Expires", expires.get_time_as_rfc7231());
      return;
    } catch (const std::length_error& e) {
      // drop through to bad request
    } catch (const std::invalid_argument& e) {
      // drop through to bad request
    } catch (const std::out_of_range& e) {
      // drop through to bad request
    } catch (const std::exception &e)  {
      std::cerr << "Failure handling request: "
                << e.what() << '\n';
      response.content << "<h1>Failed</h1>\n";
      return;
    }
  }
  response.generate_standard_response(HTTPStatus::bad_request);
}
