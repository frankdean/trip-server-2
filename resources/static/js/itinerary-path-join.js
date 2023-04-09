// -*- mode: js2; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=js norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2023 Frank Dean <frank.dean@fdsd.co.uk>

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

const GeoJSON = ol.format.GeoJSON;
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;

const pageInfo = JSON.parse(pageInfoJSON);

class FeatureMap extends TripMap {

  constructor(providers, opt_options) {
    super(providers, opt_options);
    // console.log(pageInfo);
  }

  handleUpdate(data) {
    // console.debug(data);
    const self = this;
    if (data.routes.features.length > 0) {
      const routeFeatures = new GeoJSON().readFeatures(data.routes, {
        dataProjection: 'EPSG:4326',
        featureProjection: 'EPSG:3857',
      });
      const routeSource = new VectorSource({
        features: routeFeatures,
      });
      this.routeLayer = new VectorLayer({
        source: routeSource,
        style: this.styleFunction.bind(self),
        opacity: 0.5,
      });
      this.map.addLayer(this.routeLayer);
      let totalExtent = ol.extent.createEmpty();
      // console.debug('exent before', totalExtent);
      ol.extent.extend(totalExtent, routeSource.getExtent());
      if (totalExtent[0] != Infinity)
        this.map.getView().fit(totalExtent);
    }
    if (data.tracks.features.length > 0) {
      // Using EPSG:4326 for WGS 84 from the GeoJSON format
      const trackFeatures = new GeoJSON().readFeatures(data.tracks,{
        dataProjection: 'EPSG:4326',
        featureProjection: 'EPSG:3857',
      });
      const trackSource = new VectorSource({
        features: trackFeatures,
      });
      this.trackLayer = new VectorLayer({
        source: trackSource,
        style: this.styleFunction.bind(self),
      });
      this.map.addLayer(this.trackLayer);
      let totalExtent = ol.extent.createEmpty();
      // console.debug('exent before', totalExtent);
      ol.extent.extend(totalExtent, trackSource.getExtent());
      if (totalExtent[0] != Infinity)
        this.map.getView().fit(totalExtent);
    }
  }

} // FeatureMap

const featureMap = new FeatureMap(providers,
                                  {
                                    itinerary_id: pageInfo.itinerary_id,
                                    url: server_prefix +
                                      '/rest/itinerary/features',
                                    mapDivId: 'itinerary-path-map',
                                    showExitControl: false,
                                  });
