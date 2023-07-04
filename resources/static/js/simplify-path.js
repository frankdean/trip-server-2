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
import { distance, perpendicularDistance, simplify } from './modules/geo-utils.js';

const GeoJSON = ol.format.GeoJSON;
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;

const globalPageInfo = JSON.parse(pageInfoJSON);

class SimplifyMap extends TripMap {

  constructor(providers, opt_options) {
    const eiffel_tower = {lng: 2.2945, lat: 48.85822222};
    const bt_tower = {lng: -0.1389, lat: 51.5215};
    const result = perpendicularDistance(-0.427093, 51.273458, -0.400305, 51.286216, -0.41771, 51.2837);
    // const result = perpendicularDistance(eiffel_tower.lng, eiffel_tower.lat, bt_tower.lng, bt_tower.lat, -0.41771, 51.2837);
    // console.debug('Perpendicular distance', result);
    const d = distance(-0.427093, 51.273458, -0.400305, 51.286216);
    // const d = distance(eiffel_tower.lng, eiffel_tower.lat, bt_tower.lng, bt_tower.lat);
    // console.debug('Distance', d);
    let options = opt_options || {};
    options.pageInfo = globalPageInfo;
    super(providers, options);
    this.toleranceInput = document.getElementById('tolerance');
    // this.toleranceInput.addEventListener('change', this.toleranceChangeEvent.bind(this), false);
    this.toleranceInput.addEventListener('input', this.toleranceChangeEvent.bind(this), false);
    document.getElementById('btn-save').addEventListener('click', this.save.bind(this), false);
    document.getElementById('btn-cancel').addEventListener('click', this.cancel.bind(this), false);
  }

  handleUpdate(data) {
    const self = this;
    // console.debug('data:', data);
    const name_element = document.getElementById('path-name');
    if (data.tracks.features.length > 0) {
      // Using EPSG:4326 for WGS 84 from the GeoJSON format
      const trackFeatures = new GeoJSON().readFeatures(data.tracks,{
        dataProjection: 'EPSG:4326',
        featureProjection: 'EPSG:3857',
      });
      this.trackSource = new VectorSource({
        features: trackFeatures,
      });
      this.trackLayer = new VectorLayer({
        source: self.trackSource,
        style: this.styleFunction.bind(self),
        opacity: 0.5,
      });
      const f = this.trackSource.getFeatures()[0];
      const simplifiedFeature = f.clone();
      let name = f.get('name');
      if (!name) {
        const id = f.get('id');
        if (id)
          name = 'ID: ' + f.get('id');
      }
      name_element.textContent = name;
      this.simplifiedSource = new VectorSource({
        features: [simplifiedFeature],
      });
      this.simplifiedLayer = new VectorLayer({
        source: self.simplifiedSource,
        style: self.styleFunction.bind(self),
      });
      const originalGeometry = this.trackSource.getFeatures()[0].getGeometry();
      // console.log('geometry:', originalGeometry);
      const pointCount = originalGeometry.flatCoordinates.length / originalGeometry.stride;;
      this.map.addLayer(this.trackLayer);
      this.map.addLayer(this.simplifiedLayer);
      const originalPointsEle = document.getElementById('original-point-count');
      this.currentPointsEle = document.getElementById('current-point-count');
      originalPointsEle.innerHTML = '' + pointCount;
      this.currentPointsEle.innerHTML = pointCount;
      const size = ol.extent.getSize(this.trackSource.getExtent());
      this.toleranceMax = Math.max(size[0], size[1]) / 2000000;
      this.toleranceMin = this.toleranceMax / 100000;
      this.toleranceInput.step = 1;
      this.toleranceInput.min = 0;
      this.toleranceInput.max = 250;
      // Calculate a logarithmic range
      this.logRangeBegin = Math.log(this.toleranceMin);
      this.logRangeEnd = Math.log(this.toleranceMax);
      this.logStep =
        (this.logRangeEnd - this.logRangeBegin) /
        (this.toleranceInput.max - this.toleranceInput.min + 1);
      // console.debug('Initial tolerance min, max and step', this.toleranceMin, this.toleranceMax);
    }

    if (data.routes.features.length > 0) {
      const routeFeatures = new GeoJSON().readFeatures(data.routes);
      routeFeatures.forEach(function(feature) {
        // Using EPSG:4326 for WGS 84 from the GeoJSON format
        const geometry = feature.getGeometry();
        geometry.transform('EPSG:4326', 'EPSG:3857');
      });
      this.routeSource = new VectorSource({
        features: routeFeatures,
      });

      this.routeLayer = new VectorLayer({
        source: self.routeSource,
        style: this.styleFunction.bind(self),
        opacity: 0.5,
      });
      this.map.addLayer(this.routeLayer);
    }

    if (data.waypoints.features.length > 0) {
      const waypointFeatures = new GeoJSON().readFeatures(data.waypoints);
      waypointFeatures.forEach(function(feature) {
        // Using EPSG:4326 for WGS 84 from the GeoJSON format
        const geometry = feature.getGeometry();
        geometry.transform('EPSG:4326', 'EPSG:3857');
      });
      this.waypointSource = new VectorSource({
        features: waypointFeatures,
      });
      this.waypointLayer = new VectorLayer({
        source: self.waypointSource,
        style: this.styleFunction.bind(self),
      });
      this.map.addLayer(this.waypointLayer);
    }

    let totalExtent = ol.extent.createEmpty();
    if (this.trackLayer)
      ol.extent.extend(totalExtent, this.trackLayer.getSource().getExtent());
    if (this.routeLayer)
      ol.extent.extend(totalExtent, this.routeLayer.getSource().getExtent());
    if (this.waypointLayer)
      ol.extent.extend(totalExtent, this.waypointLayer.getSource().getExtent());
    this.map.getView().fit(totalExtent);
    this.toleranceChangeEvent(new CustomEvent('startup'));
  }

  calculateTolerance() {
    return Math.exp(this.logRangeBegin + this.logStep * this.toleranceInput.value);
  }

  toleranceChangeEvent(event) {
    const self = this;
    // console.debug('tolerance change event', this.toleranceInput.value);
    const originalFeature = this.trackSource.getFeatures()[0];
    const simplifiedFeature = this.simplifiedLayer.getSource().getFeatures()[0];
    // console.debug('simplifiedFeature:', simplifiedFeature);
    const geoJson = new GeoJSON().writeFeatureObject(originalFeature,{
      dataProjection: 'EPSG:4326',
      featureProjection: 'EPSG:3857',
    });
    const epsilon = this.calculateTolerance();
    const geotype = geoJson.geometry.type;
    // console.debug('GeoJSON', geoJson);
    if (geotype === 'LineString') {
      geoJson.geometry.coordinates = simplify(geoJson.geometry.coordinates, epsilon);
    } else {
      const segments = new Array();
      geoJson.geometry.coordinates.forEach( (coords) => {
        segments.push(simplify(coords, epsilon));
      });
      geoJson.geometry.coordinates = segments;
    }
    const simplifiedFeatures = new GeoJSON().readFeatures(geoJson, {
      dataProjection: 'EPSG:4326',
      featureProjection: 'EPSG:3857',
    });
    const simplifiedGeometry = simplifiedFeatures[0].getGeometry();
    simplifiedFeature.setGeometry(simplifiedGeometry);
    const points = simplifiedGeometry.flatCoordinates.length / simplifiedGeometry.stride;
    this.currentPointsEle.innerHTML = '' + points;
  }

  save(event) {
    const self = this;
    const simplifiedFeature = this.simplifiedLayer.getSource().getFeatures()[0];
    // console.debug('simplifiedFeature:', simplifiedFeature);
    const geoJson = new GeoJSON().writeFeatureObject(simplifiedFeature,{
      dataProjection: 'EPSG:4326',
      featureProjection: 'EPSG:3857',
    });
    const data = JSON.stringify({
      'action': 'save_simplified',
      'itinerary_id': this.options.itinerary_id,
      'track': geoJson,
      'tolerance': Number.parseFloat(self.calculateTolerance()),
    });
    // console.debug('simplified feature:', data);
    const myHeaders = new Headers([
      ['Content-Type', 'application/json; charset=UTF-8']
    ]);
    const myRequest = new Request(
      self.options.saveUrl,
      {
        method: 'POST',
        body: data,
        headers: myHeaders,
        mode: 'same-origin',
        cache: 'no-store',
        referrerPolicy: 'no-referrer',
      });

    fetch(myRequest)
      .then((response) => {
        if (!response.ok) {
          throw new Error('HTTP status code: ' + response.status);
        }
        location.assign(self.options.updatesUrl);
        return;
      })
      .catch((error) => {
        alert('Failed to save track: ' + error);
        console.error('Error saving track: ', error);
      });
  }

  cancel(event) {
    location.assign(this.options.updatesUrl);
  }

} // SimplifyMap

const simplifyMap = new SimplifyMap(providers,
                                    {
                                      itinerary_id: globalPageInfo.itinerary_id,
                                      url: server_prefix +
                                        '/rest/itinerary/features',
                                      saveUrl: server_prefix +
                                        '/rest/itinerary/features',
                                      updatesUrl: `${server_prefix}/itinerary?id=${globalPageInfo.itinerary_id}&active-tab=features`,
                                      mapDivId: 'simplify-track-map',
                                      showExitControl: false,
                                    });
