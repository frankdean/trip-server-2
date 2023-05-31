// -*- mode: js2; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=js norl:
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
import { TripMap } from './modules/map.js';

const meters_per_mile = 1609.344;
const meters_per_foot = 0.0254 * 12;

const fromLonLat = ol.proj.fromLonLat;
const CircleStyle = ol.style.Circle;
const Feature = ol.Feature;
const Fill = ol.style.Fill;
const GeoJSON = ol.format.GeoJSON;
const Point = ol.geom.Point;
const Stroke = ol.style.Stroke;
const Style = ol.style.Style;
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;

class TrackingMap extends TripMap {

  constructor(providers, opt_options) {
    super(providers, opt_options);
    this.markerVisible = true;
    this.log_range_begin = Math.log(150);
    this.log_range_end = Math.log(12800);
    this.stop = false;
    this.haveInitialResult = false;
    this.lat = this.options.query_params.get('lat');
    this.lng = this.options.query_params.get('lng');
    if (this.lat && this.lng) {
      // User chose to view single point on the map
      this.stop = true;
      this.showSingleMapPoint();
    } else {
      this.update();
    }
  }

  fetchFeatures() {
    const self = this;
    this.nickname = this.options.query_params.get('nickname');
    this.endDate = new Date(this.options.query_params.get('to'));
    // console.log('To:', this.endDate);
    if (self.lat && self.lng) {
      // User chose to view single point on the map
      self.stop = true;
      self.showSingleMapPoint();
    } else {
      // console.log("URL:", self.url);
      const myHeaders = new Headers();
      const myRequest = new Request(
        self.options.url + '?' +
          new URLSearchParams({
            nickname: self.options.query_params.get('nickname'),
            from: self.options.query_params.get('from'),
            to: self.options.query_params.get('to'),
            max_hdop: self.options.query_params.get('max_hdop'),
            notes_only_flag: self.options.query_params.get('notes_only_flag'),
            order: 'ASC',
            offset: -1,
            page_size: -1,
          }),
        {
          method: 'GET',
          headers: myHeaders,
          mode: 'same-origin',
          cache: 'no-store',
          referrerPolicy: 'no-referrer',
        });

      fetch(myRequest)
        .then((response) => {
          if (!response.ok) {
            throw new Error('Request failed');
          }
          // console.log('Status:', response.status);
          return response.json();
        })
        .then((data) => {
          // console.log('data:', data);
          self.min_id_threshold = data.last_location_id !== undefined ? data.last_location_id : 0;
          self.handleUpdate(data);
          self.haveInitialResult = true;
        })
        .catch((error) => {
          console.error('Error fetching from Trip:', error);
          self.stop = true;
        });
    } // else
  } // fetchFeatures()

  handleUpdate(data) {
    const infoDiv = document.getElementById('track-info');
    if (!data.geojsonObject) {
      infoDiv.style.display = 'none';
      return;
    }
    let length = 0;
    const trackFeatures = new GeoJSON().readFeatures(data.geojsonObject);
    trackFeatures.forEach(function(feature) {
      // Using EPSG:4326 for WGS 84 from the GeoJSON format
      const geometry = feature.getGeometry();
      geometry.transform('EPSG:4326', 'EPSG:3857');
      length += ol.sphere.getLength(geometry);
    });

    const mapDiv = document.getElementById('map');
    const footerDiv = document.getElementById('footer');

    let metric = `${(length / 1000).toFixed(2)}&nbsp;km`;
    if (data.ascent)
      metric += `&nbsp;↗${data.ascent}&nbsp;m`;
    if (data.descent)
      metric += `&nbsp;↘${data.descent}&nbsp;m`;
    if (data.maxHeight  && data.minHeight) {
      metric += `&nbsp; ${data.maxHeight}⇅${data.minHeight}&nbsp;m`;
    }

    let imperial = `${(length / meters_per_mile).toFixed(2)}&nbsp;mi`;
    if (data.ascent)
      imperial += `&nbsp;↗${(data.ascent / meters_per_foot).toFixed(0)}&nbsp;ft`;
    if (data.descent)
      imperial += `&nbsp;↘${(data.descent / meters_per_foot).toFixed(0)}&nbsp;ft`;
    if (data.maxHeight  && data.minHeight) {
      imperial += `&nbsp; ${(data.maxHeight / meters_per_foot).toFixed(0)}⇅${(data.minHeight / meters_per_foot).toFixed(0)}&nbsp;ft`;
    }

    infoDiv.style.bottom = footerDiv.offsetHeight + 'px';
    infoDiv.innerHTML = `<p>${metric}</br>${imperial}`;
    infoDiv.style.background = '#e6e6e6'; //'rgb(233, 237, 242)'; // 'rgb(248, 249, 250)';
    mapDiv.style.bottom = `${infoDiv.offsetHeight + footerDiv.offsetHeight}px`;
    this.trackSource = new VectorSource({
      features: trackFeatures,
    });

    const self = this;
    let layerExisted = this.trackLayer !== undefined;
    if (this.trackLayer !== undefined)
      this.map.removeLayer(this.trackLayer);

    this.trackLayer = new VectorLayer({
      source: self.trackSource,
      style: this.styleFunction.bind(self),
    });

    if (!layerExisted)
      this.map.getView().fit(this.trackLayer.getSource().getExtent());
    this.map.addLayer(this.trackLayer);

    if (data.most_recent) {
      const container = document.getElementById('popup');
      const content = document.getElementById('popup-content');
      const closer = document.getElementById('popup-closer');

      const overlayPosition = fromLonLat(data.most_recent.position);
      const markerExists = this.markerFeature !== undefined;

      this.markerFeature = new ol.Feature({
        geometry: new Point(overlayPosition)
      });

      this.trackSource.addFeatures([
        this.markerFeature,
      ]);

      if (this.markerOverlay !== undefined) {
        this.map.removeOverlay(this.markerOverlay);
        this.markerOverlay = undefined;
      }

      this.markerOverlay = new ol.Overlay({
        element: container,
      });
      this.map.addOverlay(this.markerOverlay);
      closer.onclick = function() {
        self.markerOverlay.setPosition(undefined);
        closer.blur();
        self.markerVisible = false;
        return false;
      };
      if (data.most_recent.note) {
        this.markerContent = `${data.most_recent.time}</br>${data.most_recent.note}`;
        this.markerVisible = true;
      } else {
        this.markerContent = data.most_recent.time;
      }
      content.innerHTML = this.markerContent;

      if (this.markerVisible) {
        this.markerOverlay.setPosition(overlayPosition);
      } else {
        this.markerOverlay.setPosition(undefined);
        closer.blur();
        const mapSize = this.map.getSize();

        const extent = this.map.getView().calculateExtent(mapSize);
        const view = this.map.getView();
        const zoom = view.getZoom();
        // Calculate logarithmic scale for different zoom levels to provide a
        // buffer to the map extent, otherwise points can go off the map.
        const log_step = (this.log_range_end - this.log_range_begin) / 6;
        const margin = zoom > 15 ? 0 : Math.exp(this.log_range_begin + (16 - zoom) * log_step);
        // console.log('zoom:', zoom, 'margin:', margin);
        const bufferedExtent = ol.extent.buffer(extent, -margin);
        if (!ol.extent.containsCoordinate(bufferedExtent, overlayPosition)) {
          view.animate({center: overlayPosition});
        }
      }

      this.map.on('singleclick', function(event) {
        if (self.map.hasFeatureAtPixel(event.pixel) === true) {
          // const coordinate = event.coordinate;
          content.innerHTML = self.markerContent;
          self.markerOverlay.setPosition(overlayPosition);
          self.markerVisible = true;
        } else {
          self.markerOverlay.setPosition(undefined);
          closer.blur();
          self.markerVisible = false;
        }
      });
      setTimeout(function() {
        self.markerOverlay.panIntoView({
          autoPanAnimation: {
            duration: 250
          },
          margin: 20,
        });
      }, 200);
    }

    if (data.message && !this.alert_shown) {
      setTimeout(function() {
        self.alert_shown = true;
        alert(data.message);
      }, 2000);
    }
  }

  checkForUpdates() {
    const self = this;
    if (this.min_id_threshold !== undefined) {
      // console.log("URL:", self.options.updatesUrl);
      // console.log("Nickname:", self.nickname);
      const myHeaders = new Headers();
      const myRequest = new Request(
        self.options.updatesUrl + '?' +
          new URLSearchParams({
            nickname: self.nickname,
            min_id_threshold: self.min_id_threshold,
          }),
        {
          method: 'GET',
          headers: myHeaders,
          mode: 'same-origin',
          cache: 'no-store',
          referrerPolicy: 'no-referrer',
        });

      fetch(myRequest)
        .then((response) => {
          if (!response.ok) {
            throw new Error('Check for updates request failed');
          }
          // console.log('Status:', response.status);
          // console.log('Response body:', response.json());
          return response.json();
        })
        .then((data) => {
          // console.log('check for updates data:', data);
          if (data.available) {
            // console.log('Fetching updates');
            self.fetchFeatures();
          // } else {
            // console.log('No more updates');
          }
        })
        .catch((error) => {
          console.error('Error fetching from Trip:', error);
          // self.stop = true;
        });
    // } else {
    //  console.debug('min_id_threshold not defined');
    }
  }

  update() {
    const self = this;
    const date = new Date();
    if (date <= this.endDate) {
      setTimeout(function() {
        if (self.haveInitialResult && !self.stop) {
          // console.log('Timeout:', date.toLocaleTimeString());
          self.checkForUpdates();
        }
        if (!self.stop)
          self.update();
      }, 60000);
    // } else {
      // console.log('Date range preceeds the current date, live updates disabled');
    }
  }

  showSingleMapPoint() {
    self = this;
    const overlayPosition = fromLonLat([this.lng, this.lat]);
    let trackSource = new VectorSource({
      features: [new Feature({
        geometry: new Point(overlayPosition),
      })],
    });
    let pointStyle = new Style({
      image: new CircleStyle({
        radius: 5,
        fill: new Fill({
          color: 'red'
        }),
        stroke: new Stroke({color: 'red', width: 4})
      }),
    });
    this.trackLayer = new VectorLayer({
      source: trackSource,
      style: pointStyle,
    });

    const mapSize = this.map.getSize();
    const extent = this.map.getView().calculateExtent(mapSize);
    if (!ol.extent.containsCoordinate(extent, overlayPosition)) {
      this.this.map.getView().setCenter(overlayPosition);
    }

    // this.map.getView().fit(trackLayer.getSource().getExtent());
    let view = this.map.getView();
    view.setCenter(overlayPosition);
    view.setZoom(17);
    const container = document.getElementById('popup');
    const content = document.getElementById('popup-content');
    const closer = document.getElementById('popup-closer');
    const overlay = new ol.Overlay({
      element: container,
      autoPan: true,
      autoPanAnimation: {
        duration: 250
      }
    });
    this.map.addOverlay(overlay);
    closer.onclick = function() {
      overlay.setPosition(undefined);
      closer.blur();
      return false;
    };
    let markerContent = `${this.lat},${this.lng}`;
    content.innerHTML = markerContent;
    overlay.setPosition(overlayPosition);
    this.map.on('singleclick', function(event) {
      content.innerHTML = markerContent;
      overlay.setPosition(overlayPosition);
    });
  }

} // TrackingMap

const simplifyMap = new TrackingMap(providers,
                                    {
                                      fetch_features: (document.getElementById('track-info') !== null),
                                      query_params: (new URL(document.location)).searchParams,
                                      url: `${server_prefix}/rest/locations`,
                                      updatesUrl: `${server_prefix}/rest/locations/is-updates`,
                                      exitUrl: `${server_prefix}/tracks`,
                                      exitMessage: click_to_exit_text,
                                    });
