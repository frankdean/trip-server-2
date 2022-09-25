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
#ifndef TRIP_GET_OPTIONS_HPP
#define TRIP_GET_OPTIONS_HPP

#include "../trip-server-common/src/get_options.hpp"

namespace fdsd
{
namespace utils { class GetOptions; }
namespace trip
{

struct TripGetOptions : public utils::GetOptions {
#ifdef HAVE_GETOPT_H
  static const char short_opts[];
  static struct option long_options[];
#endif // HAVE_GETOPT_H
  static int expire_sessions;
  static int upgrade_flag;
  void confirm_force();
  virtual void show_version_info() const override;
  virtual void usage(std::ostream& os) const override;
#ifdef HAVE_GETOPT_H
  virtual bool handle_option(int c) override;
  virtual const struct option* get_long_options() const override;
  virtual const char* get_short_options() const override;
#endif // HAVE_GETOPT_H
  TripGetOptions();
};

} // namespace trip
} // namespace fdsd

#endif // TRIP_GET_OPTIONS_HPP
