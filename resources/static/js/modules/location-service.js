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

import { parseLocationText, convertToFormat } from './utils-service.js';

export { LocationParser };

class LocationParser {

  constructor(opt_options) {
    const options = opt_options || {};
    this.setSource(options.sourceElementId);
    this.setTarget(options.targetElementId);
    this.setFormatSource(options.formatElementId);
    this.setFormatStyleSource(options.formatStyleElementId, options.formatStyleDivId);
    this.setFormat(options.format);
    this.setFormatStyle(options.formatStyle);
    this.setLongitudeElement(options.lngElementId);
    this.setLatitudeElement(options.latElementId);
    this.updateLocationText();
  }

  setSource(elementId) {
    if (elementId)
      this.locationTextElement = document.getElementById(elementId);
  }

  setTarget(elementId) {
    // console.debug('Setting target element to', elementId);
    this.targetElement = document.getElementById(elementId);
    if (this.locationTextElement)
      this.locationTextElement.addEventListener('input', this.locationTextChangeEvent.bind(this), false);
    else
      console.error('LocationParser source not set');
  }

  setLongitudeElement(elementId) {
    if (!elementId) return;
    this.lngElement = document.getElementById(elementId);
  }

  setLatitudeElement(elementId) {
    if (!elementId) return;
    this.latElement = document.getElementById(elementId);
  }

  setFormatSource(elementId) {
    if (!elementId) return;
    this.formatSourceElement = document.getElementById(elementId);
    if (this.formatSourceElement)
      this.formatSourceElement.addEventListener('input', this.formatChangeEvent.bind(this), false);
    else
      console.error('LocationParser source not set');
  }

  formatChangeEvent(event) {
    // console.debug('Format:', this.formatSourceElement.value);
    this.updateFormat();
    this.updateFormatStyle();
    this.updateLocationText();
  }

  updateFormat() {
    if (this.formatSourceElement)
      this.setFormat(this.formatSourceElement.value);
  }

  setFormatStyleSource(elementId, divId) {
    if (divId)
      this.formatStyleDiv = document.getElementById(divId);
    if (!elementId) return;
    this.formatStyleElement = document.getElementById(elementId);
    if (this.formatStyleElement)
      this.formatStyleElement.addEventListener('input', this.formatStyleChangeEvent.bind(this), false);
  }

  formatStyleChangeEvent(event) {
    // console.debug('Format style:', this.formatStyleElement.value);
    this.updateFormatStyle();
    this.updateLocationText();
  }

  updateFormatStyle() {
    if (this.formatStyleElement)
      this.setFormatStyle(this.formatStyleElement.value);
    if (this.formatSourceElement) {
      const coordFormat = this.formatSourceElement.value;
      const show = coordFormat !== 'plus+code' && coordFormat !== 'osgb36' && coordFormat !== 'IrishGrid' && coordFormat !== 'ITM';
      if (show && this.formatStyleDiv) {
        this.formatStyleDiv.style.display = 'block';
      } else {
        this.formatStyleDiv.style.display = 'none';
      }
    }
  }

  setFormat(formatStr) {
    if (formatStr) {
      this.format = formatStr;
      return;
    }
    this.format = '%i%d';
    this.updateFormat();
  }

  setFormatStyle(style) {
    if (style) {
      this.formatStyle = style;
      return;
    }
    this.formatStyle = 'lat-lng';
    this.updateFormatStyle();
  }

  locationTextChangeEvent(event) {
    this.updateLocationText();
  }

  updateLocationText() {
    // console.debug('Value:', this.locationTextElement.value);
    const r = parseLocationText(this.locationTextElement.value);
    if (r !== undefined && r.lng !== undefined && r.lat !== undefined && !isNaN(r.lng) && !isNaN(r.lng)) {
      if (this.targetElement)
        this.targetElement.textContent = convertToFormat(r.lat, r.lng, this.format, this.formatStyle);
      if (this.lngElement)
        this.lngElement.value = r.lng;
      if (this.latElement)
        this.latElement.value = r.lat;
    } else {
      if (this.targetElement)
        this.targetElement.textContent = '';
      if (this.lngElement)
        this.lngElement.value = '';
      if (this.latElement)
        this.latElement.value = '';
    }
  }

} // LocationParser
