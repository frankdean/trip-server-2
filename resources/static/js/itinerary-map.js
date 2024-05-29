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

import { TripMap,
         CreateFeatureControl,
         ModifyFeatureControl,
         LocationSharingControl,
         sanitize} from './modules/map.js';

const fromLonLat = ol.proj.fromLonLat;
const Control = ol.control.Control;
const Draw = ol.interaction.Draw;
const GeoJSON = ol.format.GeoJSON;
const GeoLocation = ol.Geolocation;
const Modify = ol.interaction.Modify;
const Point = ol.geom.Point;
const Select = ol.interaction.Select;
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;
const eventCondition = ol.events.condition;
const toLonLat = ol.proj.toLonLat;

const globalPageInfo = JSON.parse(pageInfoJSON);
// console.debug('pageInfo', globalPageInfo);

class ItineraryMap extends TripMap {

  constructor(providers, opt_options) {
    let options = opt_options || {};
    options.pageInfo = globalPageInfo;
    super(providers, options);
    this.timer_count = 0;
    this.liveMapData = {
      refreshInterval: 10,
      trackingInterval: 30,
      trackMe: true,
    };
    const self = this;
    if (!this.options.readOnly) {
      const controls = this.map.getControls();

      this.createFeatureControl = new CreateFeatureControl(
        this.editFeatureEventHandler.bind(self),
        this.options);

      this.modifyFeatureControl = new ModifyFeatureControl(
        this.editFeatureEventHandler.bind(self),
        this.options);

      controls.push(this.createFeatureControl);
      controls.push(this.modifyFeatureControl);
    }
    this.tracking = false;
    this.liveLocationsMap = new Map();
      this.geoLocation = new GeoLocation({
        tracking: false,
        trackingOptions: {
          enableHighAccuracy: true,
        },
        projection: this.map.getView().getProjection(),
      });
    document
      .querySelectorAll('.ol-zoom-in, .ol-zoom-out, .ol-rotate-reset, .select-map-layer, .map-route-control, .map-waypoint-control, .map-edit-control, .map-delete-control')
      .forEach(function (el) {
        new bootstrap.Tooltip(el, {
          container: '#map',
        });
      });
  }

  handleUpdate(data) {
    const self = this;
    if (data.locationSharers.length > 0) {
      this.options.nicknames = data.locationSharers;
      this.options.pathColors = data.pathColors;
      if (this.locationSharingControl)
        this.map.getControls().remove(this.locationSharingControl);
      this.locationSharingControl = new LocationSharingControl(
        self.handleLocationSharingEvent.bind(self),
        this.options,
      );
      this.map.getControls().push(this.locationSharingControl);
      document
        .querySelectorAll('.map-live-control')
        .forEach(function (el) {
          new bootstrap.Tooltip(el, {
            container: '#map',
          });
        });
    }
    //console.debug('data:', data);
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
      opacity: 0.5,
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

  handleLocationSharingEvent(event) {
    if (event.detail)
      this.liveMapData = event.detail;
    if (event.detail && event.detail.refreshInterval !== undefined)
      this.liveMapData.refreshInterval = event.detail.refreshInterval;
    if (event.detail && event.detail.trackingInterval !== undefined)
      this.liveMapData.trackingInterval = event.detail.trackingInterval;
    if (event.detail && event.detail.trackMe !== undefined)
      this.liveMapData.trackMe = event.detail.trackMe;
    switch(event.type) {
    case 'start':
      this.liveLocationsMap = new Map();
      this.stop = false;
      this.checkForUpdates();
      this.update();
      break;
    case 'stop':
      this.stop = true;
      this.liveMapData.trackMe = false;
      this.updateTracking();
      break;
    case 'cancel':
      break;
    default:
      console.error('Unexpected create feature event type', event.type);
    }
    this.locationSharingControl.setStopState(this.stop);
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
        this.saveFeature(value);
      }
      this.modifiedRoutes.clear();
    }
    // waypoints
    if (this.modifiedWaypoints && this.modifiedWaypoints.size > 0) {
      for (const [key, value] of this.modifiedWaypoints.entries()) {
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

    if (this.deletedRoutes)
      this.deletedRoutes.length = 0;
    if (this.deletedWaypoints)
      this.deletedWaypoints.length = 0;
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
    // console.debug('abortFeatureEdit');
    this.removeInteractions();
    this.refetch = false;
    if (this.modifiedRoutes && this.modifiedRoutes.size > 0) {
      console.debug('Clearing map of modified routes');
      this.refetch = true;
      this.modifiedRoutes.clear();
    }
    if (this.modifiedWaypoints && this.modifiedWaypoints.size > 0) {
      console.debug('Clearing map of modified waypoints');
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
      console.debug('Clearing all mapping features and refetching');
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
    // console.debug('Saving feature pageInfo', this.options.pageInfo);
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
          feature.set('id', data.id);
          const geometry = feature.getGeometry();
          // Add id to list of features so they are included when we re-fetch objects.
          switch (feature.get('type')) {
          case 'route':
            self.options.pageInfo.features.routes.push(data.id);
            break;
          case 'waypoint':
            self.options.pageInfo.features.waypoints.push(data.id);
            break;
          default:
            console.error('Unexpected feature type:', feature.get('type'));
          }
        }
      })
      .catch((error) => {
        console.error('Error saving route: ', error);
        alert('Failed to save route: ' + error);
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

  displayLiveLocations() {
    const self = this;
    let dirty = false;
    if (self.locationLayer !== undefined) {
      // console.debug('Removing existing layer');
      self.map.removeLayer(self.locationLayer);
      self.locationLayer = undefined;
    }
    // console.debug('liveLocationsMap', this.liveLocationsMap);
    const trackSource = new VectorSource({
      wrapX: false,
    });
    self.locationLayer = new VectorLayer({
      source: trackSource,
      style: self.styleFunction.bind(self),
    });
    self.map.addLayer(self.locationLayer);
    this.liveLocationsMap.forEach((value, nickname, map) => {
      // console.debug('Live locations map value', nickname, value);
      const trackFeatures = new GeoJSON().readFeatures(value.geojsonObject, {
        dataProjection: 'EPSG:4326',
        featureProjection: 'EPSG:3857',
      });
      if (!dirty && trackFeatures.length > 0)
        dirty = true;
      trackSource.addFeatures(trackFeatures);
      const features = value.geojsonObject.features;
      const name = value.most_recent && value.most_recent.note ? nickname + ' (' + value.most_recent.note + ')' : nickname;
      if (trackSource.getFeatures().length == 1 && trackSource.getFeatures()[0].getGeometry().getType() == 'Point') {
        const feature = trackSource.getFeatures()[0];
        feature.set('name', name);
      } else if (features.length > 0) {
        const coords = features[features.length - 1].geometry.coordinates;
        trackSource.addFeature(
          new ol.Feature({
            geometry: new Point(fromLonLat(coords[coords.length - 1])),
            name: name,
          })
        );
      }
    });
    if (dirty && !this.map_initialized) {
      this.map_initialized = true;
      let totalExtent = ol.extent.createEmpty();
      if (self.trackLayer)
        ol.extent.extend(totalExtent, self.trackLayer.getSource().getExtent());
      if (self.routeLayer)
        ol.extent.extend(totalExtent, self.routeLayer.getSource().getExtent());
      if (self.waypointLayer)
        ol.extent.extend(totalExtent, self.waypointLayer.getSource().getExtent());
      if (self.locationLayer)
        ol.extent.extend(totalExtent, self.locationLayer.getSource().getExtent());
      // console.debug('Extent', totalExtent[0]);
      if (totalExtent[0] != Infinity)
        self.map.getView().fit(totalExtent);
    }
  }

  fetchLiveLocationUpdates(nicknames) {
    // console.debug('fetchLiveLocationUpdates', this.liveMapData);
    const self = this;
    const max_hdop = Number(this.liveMapData.maxHdop);
    const data = JSON.stringify({
      nicknames: nicknames,
      max_hdop: max_hdop == 0 ? -1 : max_hdop,
      from: this.liveMapData.start,
    });
    const myHeaders = new Headers([
      ['Content-Type', 'application/json; charset=UTF-8']
    ]);
    const myRequest = new Request(
      self.options.liveLocationsUrl,
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
          throw new Error('Fetch live location updates failed');
        }
        // console.log('Status:', response.status);
        // console.log('Response body:', response.json());
        return response.json();
      })
      .then((data) => {
        data.forEach((e) => {
          // console.debug('Received update for', e.nickname);
          if (e.locations) {
            const m = self.liveMapData.nicknames.get(e.nickname);
            if (m) {
              const features = e.locations.geojsonObject.features;
              if (e.locations.geojsonObject.features.length > 0) {
                features.forEach((feature) => {
                  feature.properties.color_code = m.pathColor.key;
                  feature.properties.html_color_code = m.pathColor.html_code;
                });
              }
              self.liveLocationsMap.set(e.nickname, e.locations);
            }
          }
        });
        self.displayLiveLocations();
      })
      .catch((error) => {
        console.error('Error fetching live location updates:', error);
      });
  }

  checkForUpdates() {
    const self = this;
    this.updateTracking();
    if (this.liveMapData && this.liveMapData.nicknames) {
      let nicknames = new Array();
      this.liveMapData.nicknames.forEach((value, key, map) => {
        if (value.selected) {
          const locationData = self.liveLocationsMap.get(key);
          let min_id_threshold =
              (locationData && locationData.last_location_id) ?
              locationData.last_location_id : undefined;
          nicknames.push({
            nickname: key,
            min_id_threshold: min_id_threshold,
          });
        }
      });
      if (nicknames.length == 0)
        return;
      const data = JSON.stringify({
        nicknames: nicknames,
        from: this.liveMapData.start,
      });
      const myHeaders = new Headers([
        ['Content-Type', 'application/json; charset=UTF-8']
      ]);
      const myRequest = new Request(
        self.options.updatesUrl,
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
            throw new Error('Check for updates request failed');
          }
          // console.log('Status:', response.status);
          // console.log('Response body:', response.json());
          return response.json();
        })
        .then((data) => {
          // console.debug('check for updates response data:', data);
          const nicknames = new Array();
          data.forEach((e) => {
            // console.debug('data entry', e);
            if (e.update_available)
              nicknames.push(e.nickname);
          });
          if (nicknames.length > 0)
            self.fetchLiveLocationUpdates(nicknames);
        })
        .catch((error) => {
          console.error('Error fetching from Trip:', error);
          // self.stop = true;
        });
    }
  }

  recordLocation() {
    const self = this;
    const now = new Date();
    // if (self.lastPositionDate) {
    //   console.debug('last', self.lastPositionDate.getTime());
    //   console.debug('now ', now.getTime());
    // }
    const age = self.lastPositionDate ? (now.getTime() - self.lastPositionDate.getTime()) : 0;
    const next = self.liveMapData.trackingInterval * 1000;
    // console.debug('Age: ', age);
    // console.debug('Next:', next);
    if (self.lastPositionDate !== undefined && age < next) {
      // console.debug('Ignoring location as too soon since last');
      return;
    }
    self.lastPositionDate = now;
    const coords = toLonLat(self.geoLocation.getPosition());
    const position =
          {
            uuid: this.options.loggingUUID,
            lng: '' + coords[0],
            lat: '' + coords[1],
            time: now.toISOString(),
            // sat: '',
            prov: 'web-browser',
            // batt: '',
            // note: '',
          };
    const hdop = self.geoLocation.getAccuracy();
    if (hdop !== undefined)
      position.hdop = '' + hdop;
    const altitude = self.geoLocation.getAltitude();
    if (altitude !== undefined)
      position.altitude = '' + altitude;
    const speed = self.geoLocation.getSpeed();
    if (speed !== undefined)
      position.speed = '' + speed;
    const bearing = self.geoLocation.getHeading();
    if (bearing !== undefined)
      position.bearing = '' + bearing;

    // console.debug('Logging', position);
    const myHeaders = new Headers([
      ['Content-Type', 'application/json; charset=UTF-8']
    ]);
    const myRequest = new Request(
      self.options.trackingUrl,
      {
        method: 'POST',
        body: JSON.stringify(position),
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
        // alert('Failed to log tracked location: ' + error);
        console.error('Failed to log tracked location: ', error);
      });
  }

  trackingListener() {
    this.recordLocation();
  }

  trackingErrorHandler(error) {
    console.error('Tracking error', error);
    // window.console.log(error);
  }

  updateTracking() {
    const self = this;
    if (!this.liveMapData)
      console.error('liveMapData is not null/undefined', this.liveMapData);
    if (this.liveMapData.trackMe && !self.tracking) {
      self.tracking = true;
      this.geoLocation.setTracking(self.tracking);
      // console.debug('Start tracking', new Date());
      this.geoLocation.on('change:position', this.trackingListener.bind(this));
      this.geoLocation.on('error', this.trackingErrorHandler.bind(this));
    } else if (self.tracking && !this.liveMapData.trackMe) {
      self.tracking = false;
      this.geoLocation.setTracking(self.tracking);
      this.geoLocation.un('change:position', self.trackingListener.bind(this));
      this.geoLocation.un('error', self.trackingErrorHandler.bind(this));
      // console.debug('Stopped tracking');
    // } else if (self.tracking && this.liveMapData.trackMe) {
    //   console.debug('Could get a new location');
    // } else {
    //   console.debug('Nothing to do');
    }
  }

  update() {
    const self = this;
    if (!this.liveMapData) {
      return;
    }
    if (!this.stop) {
      if (this.timer_count < 1) {
        this.timer_count++;
        // console.debug('Setting refresh interval to', this.liveMapData.refreshInterval);
        setTimeout(function() {
          self.timer_count--;
          if (!self.stop) {
            // console.debug('Timeout', new Date());
            self.checkForUpdates();
            self.update();
          }
        }, this.liveMapData.refreshInterval * 1000);
      }
    }
  }

} // class TripMap

const itineraryMap = new ItineraryMap(providers,
                                      {
                                        itinerary_id: globalPageInfo.itinerary_id,
                                        url: server_prefix +
                                          '/rest/itinerary/features',
                                        trackingUrl: server_prefix +
                                          '/rest/log_point',
                                        loggingUUID: logging_uuid,
                                        saveUrl: server_prefix +
                                          '/rest/itinerary/features',
                                        liveLocationsUrl: `${server_prefix}/rest/locations`,
                                        updatesUrl: `${server_prefix}/rest/locations/is-updates`,
                                        exitUrl: `${server_prefix}/itinerary?id=${globalPageInfo.itinerary_id}&active-tab=features`,
                                        readOnly: readOnly,
                                        exitMessage: click_to_exit_text,
                                      });
