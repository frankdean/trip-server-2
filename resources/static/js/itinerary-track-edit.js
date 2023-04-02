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
import { SelectionHandler } from './modules/itinerary.js';

const GeoJSON = ol.format.GeoJSON;
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;

const pageInfo = JSON.parse(pageInfoJSON);

class FeatureMap extends TripMap {

  constructor(providers, opt_options) {
    super(providers, opt_options);
    const selectHandler = new SelectionHandler(this.segmentChange.bind(this));
    // console.log(pageInfo);
    const all = selectHandler.isSelectAll();
    let segments = pageInfo.segments;
    this.segmentMap = new Map();
    segments.forEach(x => this.segmentMap.set(x, all));
    // console.log(this.segmentMap);
  }

  handleUpdate(data) {
    const self = this;
    if (data.tracks.features.length > 0) {
      // Using EPSG:4326 for WGS 84 from the GeoJSON format
      this.trackFeatures = new GeoJSON().readFeatures(data.tracks,{
        dataProjection: 'EPSG:4326',
        featureProjection: 'EPSG:3857',
      });
      this.updateMapLayer();
      const source = new VectorSource({
        features: self.trackFeatures,
      });
      let totalExtent = ol.extent.createEmpty();
      // console.debug('exent before', totalExtent);
      ol.extent.extend(totalExtent, source.getExtent());
      if (totalExtent[0] != Infinity)
        this.map.getView().fit(totalExtent);
    }
  }

  updateMapLayer() {
    const self = this;
    // console.log(this.trackFeatures);
    const selectedFeatures = new Array();
    this.trackFeatures.forEach((feature) => {
      const id = feature.get('id');
      if (this.segmentMap.has(id)) {
        const segment = this.segmentMap.get(id);
        if (segment)
          selectedFeatures.push(feature);
      }
    });
    if (this.trackLayer) {
      this.map.removeLayer(this.trackLayer);
      this.trackLayer = undefined;
    }
    this.trackSource = new VectorSource({
      features: selectedFeatures,
    });
    this.trackLayer = new VectorLayer({
      source: self.trackSource,
      style: this.styleFunction.bind(self),
    });
    // let totalExtent = ol.extent.createEmpty();
    // if (this.trackLayer)
    //   ol.extent.extend(totalExtent, this.trackSource.getExtent());
    this.map.addLayer(this.trackLayer);
    // this.map.getView().fit(totalExtent);
  }

  segmentChange(segmentId, state) {
    // console.debug('Segment', segmentId, state);
    this.segmentMap.set(Number(segmentId), state);
    // console.log(this.segmentMap);
    this.updateMapLayer();
  }

} // FeatureMap

const featureMap = new FeatureMap(providers,
                                  {
                                    itinerary_id: pageInfo.itinerary_id,
                                    url: server_prefix +
                                      '/rest/itinerary/features',
                                    mapDivId: 'itinerary-track-map',
                                    showExitControl: false,
                                  });
