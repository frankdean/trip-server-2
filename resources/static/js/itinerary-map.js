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

import { TripMap, CreateFeatureControl, ModifyFeatureControl } from './modules/map.js';

const Control = ol.control.Control;
const Draw = ol.interaction.Draw;
const GeoJSON = ol.format.GeoJSON;
const Modify = ol.interaction.Modify;
const Select = ol.interaction.Select;
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;
const eventCondition = ol.events.condition;

const pageInfo = JSON.parse(pageInfoJSON);
// console.debug('pageInfo', pageInfo);

class ItineraryMap extends TripMap {

  constructor(providers, opt_options) {
    super(providers, opt_options);
    const options = opt_options || {};
    const self = this;
    if (!options.readOnly) {
      const controls = this.map.getControls();

      this.createFeatureControl = new CreateFeatureControl(
        this.editFeatureEventHandler.bind(self),
        options);

      this.modifyFeatureControl = new ModifyFeatureControl(
        this.editFeatureEventHandler.bind(self),
        options);

      controls.push(this.createFeatureControl);
      controls.push(this.modifyFeatureControl);
    }
  }

  handleUpdate(data) {
    const self = this;
    // console.debug('data:', data);
    this.trackSource = new VectorSource({
      wrapX: false,
    });
    this.trackLayer = new VectorLayer({
      source: self.trackSource,
      style: this.styleFunction.bind(self),
    });
    this.map.addLayer(this.trackLayer);
    if (data.tracks.features.length > 0) {
      // Using EPSG:4326 for WGS 84 from the GeoJSON format
      const trackFeatures = new GeoJSON().readFeatures(data.tracks,{
        dataProjection: 'EPSG:4326',
        featureProjection: 'EPSG:3857',
      });
      this.trackSource.addFeatures(trackFeatures);
    }

    this.routeSource = new VectorSource();
    this.routeLayer = new VectorLayer({
      source: self.routeSource,
      style: this.styleFunction.bind(self),
      opacity: 0.3,
    });
    this.map.addLayer(this.routeLayer);
    if (data.routes.features.length > 0) {
      const routeFeatures = new GeoJSON().readFeatures(data.routes);
      routeFeatures.forEach(function(feature) {
        // Using EPSG:4326 for WGS 84 from the GeoJSON format
        const geometry = feature.getGeometry();
        geometry.transform('EPSG:4326', 'EPSG:3857');
      });
      this.routeSource.addFeatures(routeFeatures);
    }
    this.routeSource.on('addfeature', function(event) {
      const type = event.feature.get('type');
      // console.debug('routeSource addfeature event', event.feature);
      const geometry = event.feature.getGeometry();
      const pointCount = geometry.flatCoordinates.length / geometry.stride;
      if (pointCount > 1)
        self.saveFeature(event.feature);
    });

    this.waypointSource = new VectorSource();
    this.waypointLayer = new VectorLayer({
      source: self.waypointSource,
      style: this.styleFunction.bind(self),
    });
    this.map.addLayer(this.waypointLayer);
    if (data.waypoints.features.length > 0) {
      const waypointFeatures = new GeoJSON().readFeatures(data.waypoints);
      waypointFeatures.forEach(function(feature) {
        // Using EPSG:4326 for WGS 84 from the GeoJSON format
        const geometry = feature.getGeometry();
        geometry.transform('EPSG:4326', 'EPSG:3857');
      });
      this.waypointSource.addFeatures(waypointFeatures);
    }
    this.waypointSource.on('addfeature', function(event) {
      event.feature.set('type', 'waypoint');
      // console.debug('waypointSource addfeature event', event.feature);
      self.saveFeature(event.feature);
      self.waypointDraw.finishDrawing();
      self.map.removeInteraction(self.waypointDraw);
      self.waypointDraw = undefined;
      self.createFeatureControl.hideOptionButtons();
    });

    if (!this.refetch) {
      let totalExtent = ol.extent.createEmpty();
      if (this.trackLayer)
        ol.extent.extend(totalExtent, this.trackLayer.getSource().getExtent());
      if (this.routeLayer)
        ol.extent.extend(totalExtent, this.routeLayer.getSource().getExtent());
      if (this.waypointLayer)
        ol.extent.extend(totalExtent, this.waypointLayer.getSource().getExtent());
      // console.debug('Extent', totalExtent[0]);
      if (totalExtent[0] != Infinity)
        this.map.getView().fit(totalExtent);
    }
  }

  editFeatureEventHandler(event) {
    switch(event.type) {

    case 'startroute':
      this.startCreateRoute();
      break;

    case 'edit':
      this.editRoute();
      break;

    case 'delete-select':
      this.selectItemsForDeletion();
      break;

    case 'save':
      this.saveModifiedFeatures();
      break;

    case 'createwaypoint':
      this.createWaypoint();
      break;

    case 'routecreated':
      this.routeDraw.finishDrawing();
      this.routeDraw = undefined;
      break;

    case 'abort':
      this.abortFeatureEdit();
      break;

    case 'undo':
      this.routeDraw.removeLastPoint();
      break;

    default:
      console.error('Unexpected create feature event type', event.type);
    }
  }

  startCreateRoute() {
    const self = this;
    this.routeDraw = new Draw({
      source: this.routeSource,
      type: 'LineString',
    });
    this.map.addInteraction(this.routeDraw);
    // Called when the user clicks on the last point or clicks the Finish button
    this.routeDraw.on('drawend', function(event) {
      // console.debug('Draw drawend', event.feature);
      event.feature.set('type', 'route');
      self.routeDraw.finishDrawing();
      self.createFeatureControl.hideOptionButtons();
      self.map.removeInteraction(self.routeDraw);
      self.routeDraw = undefined;
    });
  }

  editRoute() {
    const self = this;
    this.modifyRoute = new Modify({
      source: this.routeSource,
      deleteCondition: (event) => {
        return (eventCondition.doubleClick(event) && eventCondition.touchOnly(event)) ||
          (eventCondition.altKeyOnly(event) && eventCondition.singleClick(event));
      },
    });
    this.map.addInteraction(this.modifyRoute);

    this.modifyRoute.on('modifystart', function(event) {
      // console.debug('Modify modifystart', event.target);
    });
    this.modifyRoute.on('modifyend', function(event) {
      // console.debug('Modify modifyend', event);
      if (!self.modifiedRoutes)
        self.modifiedRoutes = new Map();
      event.features.forEach((route) => {
        const id = route.get('id');
        // console.debug('Modified route', id);
        self.modifiedRoutes.set(id, route);
      });
    });

    // Waypoints
    this.modifyWaypoint = new Modify({
      source: this.waypointSource,
      type: 'Point',
    });
    this.map.addInteraction(this.modifyWaypoint);

    this.modifyWaypoint.on('modifyend', function(event) {
      // console.debug('Modify modifyend', event);
      if (!self.modifiedWaypoints)
        self.modifiedWaypoints = new Map();
      event.features.forEach((waypoint) => {
        const id = waypoint.get('id');
        // console.debug('Modified waypoint', id);
        self.modifiedWaypoints.set(id, waypoint);
      });
    });
  }

  selectItemsForDeletion() {
    const self = this;
    this.selectFeatures = new Select({
      layers: [self.routeLayer, self.waypointLayer],
    });
    this.map.addInteraction(this.selectFeatures);
    this.selectFeatures.on('select', (event) => {
      if (!self.deletedRoutes)
        self.deletedRoutes = new Array();
      if (!self.deletedWaypoints)
        self.deletedWaypoints = new Array();
      event.selected.forEach((feature) => {
        const type = feature.get('type');
        const id = feature.get('id');
        if (id) {
          switch (type) {
          case 'route':
            self.deletedRoutes.push(id);
            self.routeSource.removeFeature(feature);
            break;
          case 'waypoint':
            self.deletedWaypoints.push(id);
            self.waypointSource.removeFeature(feature);
            break;
          default :
            console.error('Seletected feature has unknown type: ', type);
            break;
          }
        } else {
          console.error('Selected feature has no ID property');
        }
      });
    });
  }

  saveModifiedFeatures() {
    if (this.modifiedRoutes && this.modifiedRoutes.size > 0) {
      for (const [key, value] of this.modifiedRoutes.entries()) {
        // console.debug('Saving feature ID:', key);
        this.saveFeature(value);
      }
      this.modifiedRoutes.clear();
    }
    // waypoints
    if (this.modifiedWaypoints && this.modifiedWaypoints.size > 0) {
      for (const [key, value] of this.modifiedWaypoints.entries()) {
        // console.debug('Saving feature ID:', key);
        this.saveFeature(value);
      }
      this.modifiedWaypoints.clear();
    }
    let selectedFeatures = {
      routes: [],
      waypoints: [],
      tracks: [],
    };
    if (this.deletedRoutes && this.deletedRoutes.length > 0)
      selectedFeatures.routes = this.deletedRoutes;
    if (this.deletedWaypoints && this.deletedWaypoints.length > 0)
      selectedFeatures.waypoints = this.deletedWaypoints;

    if (selectedFeatures.routes.length > 0 ||
        selectedFeatures.waypoints.length > 0)
      this.deleteFeatures(selectedFeatures);
    this.removeInteractions();
  }

  createWaypoint() {
    this.waypointDraw = new Draw({
      source: this.waypointSource,
      type: 'Point',
    });
    this.map.addInteraction(this.waypointDraw);
  }

  abortFeatureEdit() {
    this.removeInteractions();
    this.refetch = false;
    if (this.modifiedRoutes && this.modifiedRoutes.size > 0) {
      this.refetch = true;
      this.modifiedRoutes.clear();
    }
    if (this.modifiedWaypoints && this.modifiedWaypoints.size > 0) {
      this.refetch = true;
      this.modifiedWaypoints.clear();
    }
    if (this.deletedRoutes && this.deletedRoutes.length > 0) {
      this.refetch = true;
      this.deletedRoutes.length = 0;
    }
    if (this.deletedWaypoints && this.deletedWaypoints.length > 0) {
      this.refetch = true;
      this.deletedWaypoints.length = 0;
    }
    this.createFeatureControl.hideOptionButtons();
    if (this.refetch) {
      this.routeSource.clear();
      this.waypointSource.clear();
      this.trackSource.clear();
      this.fetchFeatures();
    }
  }

  removeInteractions() {
    if (this.routeDraw) {
      this.routeDraw.abortDrawing();
      this.map.removeInteraction(this.routeDraw);
      this.routeDraw = undefined;
    }
    if (this.waypointDraw) {
      this.waypointDraw.abortDrawing();
      this.map.removeInteraction(this.waypointDraw);
      this.waypointDraw = undefined;
    }
    if (this.modifyRoute) {
      this.map.removeInteraction(this.modifyRoute);
      this.modifyRoute = undefined;
    }
    if (this.modifyWaypoint) {
      this.map.removeInteraction(this.modifyWaypoint);
      this.modifyWaypoint = undefined;
    }
    if (this.selectFeatures) {
      this.map.removeInteraction(this.selectFeatures);
      this.selectFeatures = undefined;
    }
  }

  saveFeature(feature) {
    const self = this;
    const geoJson = new GeoJSON().writeFeatureObject(feature,{
      dataProjection: 'EPSG:4326',
      featureProjection: 'EPSG:3857',
    });
    const data = JSON.stringify({
      'action': 'create-map-feature',
      'itinerary_id': this.options.itinerary_id,
      'feature': geoJson,
    });
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
        // console.debug('Response', response.text());
        return response.json();
      })
      .then((data) => {
        // console.debug('Data response', data);
        if (data.id && !feature.get('id') ) {
          // console.debug('Setting id to', data.id);
          feature.set('id', data.id);
        }
      })
      .catch((error) => {
        alert('Failed to save route: ' + error);
        console.error('Error saving route: ', error);
      });
  }

  deleteFeatures(selectedFeatures) {
    const self = this;
    const data = JSON.stringify({
      'action': 'delete-features',
      'itinerary_id': this.options.itinerary_id,
      'features': selectedFeatures,
    });
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
        return;
      })
      .catch((error) => {
        alert('Failed to delete features: ' + error);
        console.error('Error deleting features: ', error);
      });
  }

}

const itineraryMap = new ItineraryMap(providers,
                                      {
                                        itinerary_id: pageInfo.itinerary_id,
                                        url: server_prefix +
                                          '/rest/itinerary/features',
                                        saveUrl: server_prefix +
                                          '/rest/itinerary/features',
                                        exitUrl: `${server_prefix}/itinerary?id=${pageInfo.itinerary_id}&active-tab=features`,
                                        readOnly: readOnly,
                                        exitMessage: click_to_exit_text,
                                      });
