// -*- mode: js2; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=js norl:
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

import { LocationParser } from './modules/location-service.js';

new LocationParser({
  sourceElementId: 'input-position',
  targetElementId: 'position-text',
  formatElementId: 'input-coord-format',
  formatStyleDivId: 'position-style-div',
  formatStyleElementId: 'position-separator',
  lngElementId: 'input-lng',
  latElementId: 'input-lat',
});

// console.debug('Proj4', proj4);
// console.debug('OLC', OpenLocationCode);
