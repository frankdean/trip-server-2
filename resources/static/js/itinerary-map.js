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

const GeoJSON = ol.format.GeoJSON;
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;

const pageInfo = JSON.parse(pageInfoJSON);
// console.debug('pageInfo', pageInfo);

class ItineraryMap extends TripMap {

  handleUpdate(data) {
    const self = this;
    // console.debug('data:', data);
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
      });
      this.map.addLayer(this.trackLayer);
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
        opacity: 0.3,
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
  }
}

const itineraryMap = new ItineraryMap(providers,
                                      {
                                        itinerary_id: pageInfo.itinerary_id,
                                        url: server_prefix +
                                          '/rest/itinerary/features',
                                        exitUrl: `${server_prefix}/itinerary?id=${pageInfo.itinerary_id}&active-tab=features`,
                                        exitMessage: click_to_exit_text,
                                      });
