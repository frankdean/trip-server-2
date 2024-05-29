// -*- mode: js2; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=js norl:
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
import { TripMap } from './modules/map.js';
import { SelectionHandler } from './modules/itinerary.js';

const GeoJSON = ol.format.GeoJSON;
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;

const globalPageInfo = JSON.parse(pageInfoJSON);

class FeatureMap extends TripMap {

  constructor(providers, opt_options) {
    let options = opt_options || {};
    options.pageInfo = globalPageInfo;
    super(providers, options);
    const selectHandler = new SelectionHandler(this.pointChange.bind(this));
    const all = selectHandler.isSelectAll();
    let point_ids = this.options.pageInfo.track_point_ids;
    this.pointIdMap = new Map();
    point_ids.forEach(x => this.pointIdMap.set(x, all));
    // console.debug('Finished FeatureMap constructor');
  }

  handleUpdate(data) {
    // console.log(data);
    const self = this;
    if (data.tracks.features.length > 0) {
      // Using EPSG:4326 for WGS 84 from the GeoJSON format
      this.trackFeatures = new GeoJSON().readFeatures(data.tracks,{
        dataProjection: 'EPSG:4326',
        featureProjection: 'EPSG:3857',
      });
      // console.debug('There are', data.waypoints.features.length, ' waypoints');
      this.waypointFeatures = new GeoJSON().readFeatures(data.waypoints,{
        dataProjection: 'EPSG:4326',
        featureProjection: 'EPSG:3857',
      });
      this.trackSource = new VectorSource({
        features: self.trackFeatures,
      });
      this.trackLayer = new VectorLayer({
        source: self.trackSource,
        style: this.styleFunction.bind(self),
      });
      this.map.addLayer(this.trackLayer);

      const source = new VectorSource({
        features: self.trackFeatures,
      });
      let totalExtent = ol.extent.createEmpty();
      this.updateMapLayer(totalExtent);
      // ol.extent.extend(totalExtent, source.getExtent());
      if (totalExtent[0] != Infinity)
        this.map.getView().fit(totalExtent);
        this.map.getView().adjustResolution(2);
    }
  }

  updateMapLayer(extent) {
    const self = this;
    if (this.waypointLayer) {
      this.map.removeLayer(this.waypointLayer);
      this.waypointLayer = undefined;
    }
    const selectedFeatures = new Array();
    this.waypointFeatures.forEach((feature) => {
      if (extent !== undefined)
        ol.extent.extend(extent, feature.getGeometry().getExtent());
      const id = feature.get('id');
      if (this.pointIdMap.has(id)) {
        const point = this.pointIdMap.get(id);
        if (point)
          selectedFeatures.push(feature);
      }
    });
    this.waypointSource = new VectorSource({
      features: selectedFeatures,
    });
    this.waypointLayer = new VectorLayer({
      source: self.waypointSource,
      style: this.styleFunction.bind(self),
    });
    this.map.addLayer(this.waypointLayer);
  }

  pointChange(pointId, state) {
    // console.debug('Point ', pointId, ' changed to' , state);
    this.pointIdMap.set(Number(pointId), state);
    // console.log(this.pointIdMap);
    this.updateMapLayer();
  }

} // FeatureMap

const featureMap = new FeatureMap(providers,
                                  {
                                    itinerary_id: globalPageInfo.itinerary_id,
                                    url: server_prefix +
                                      '/rest/itinerary/features',
                                    mapDivId: 'itinerary-track-map',
                                    showExitControl: false,
                                  });
