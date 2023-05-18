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

export { TripMap,
         CreateFeatureControl,
         ModifyFeatureControl,
         LocationSharingControl,
         sanitize};

const fromLonLat = ol.proj.fromLonLat;
const CircleStyle = ol.style.Circle;
const Control = ol.control.Control;
const defaultControls = ol.control.defaults.defaults;
const Feature = ol.Feature;
const Fill = ol.style.Fill;
// const GeoJSON = ol.format.GeoJSON;
const Icon = ol.style.Icon;
const Link = ol.interaction.Link;
const LineString = ol.geom.LineString;
const OSM = ol.source.OSM;
const Point = ol.geom.Point;
const RegularShape = ol.style.RegularShape;
const Stroke = ol.style.Stroke;
const Style = ol.style.Style;
const TextStyle = ol.style.Text;
const TileLayer = ol.layer.Tile;
// const VectorLayer = ol.layer.Vector;
// const VectorSource = ol.source.Vector;
const View = ol.View;
const XYZ = ol.source.XYZ;

const sanitize = function(s) {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
    .replace(/"/g, '&quot:').replace(/'/g, '&#039;');
};

class SelectMapLayerControl extends Control {

  constructor(mapLayerManager, opt_options) {
    const options = opt_options || {};

    const button = document.createElement('button');
    button.className = 'map-layers-control';

    const element = document.createElement('div');
    element.id = 'select-map-control';
    element.className = 'select-map-layer ol-unselectable ol-control';
    element.setAttribute('data-bs-original-title', 'Select map');
    element.appendChild(button);

    super({
      element: element,
      target: options.target,
    });
    this.mapLayerManager = mapLayerManager;
    this.options = options;

    button.addEventListener('mouseover', this.handleSelectMapLayer.bind(this), false);
    element.addEventListener('mouseleave', this.handleHideModal.bind(this), false);
  }

  handleSelectMapLayer() {
    this.modal = new SelectMapModal(this.mapLayerManager, this.options);
  }

  handleHideModal() {
    if (this.modal) {
      this.modal.hide();
    }
  }
}

class SelectMapModal {

  constructor(mapLayerManager, opt_options) {
    this.options = opt_options || {};
    this.mapLayerManager = mapLayerManager;
    const outerdiv = document.createElement('div');
    outerdiv.id = 'map-layer-list-container';
    const formElement = document.createElement('div');
    formElement.id = 'map-layer-list';
    formElement.className = 'map-layer-modal';
    let count = 0;
    const self = this;
    this.mapLayerManager.providers.forEach(function(p) {
      const label = document.createElement('label');
      label.htmlFor = 'provider-' + count;
      label.innerHTML = sanitize(p.name);
      label.className = 'map-provider';
      const input = document.createElement('input');
      input.id = 'provider-' + count;
      input.type = 'radio';
      input.name = 'layer';
      input.className = 'map-provider';
      input.value = count;
      if (self.mapLayerManager.currentProviderIndex == count) {
        input.checked = true;
      }
      input.addEventListener('click', self.handleSelectMapLayer.bind(self), false);
      formElement.appendChild(input);
      formElement.appendChild(label);
      formElement.appendChild(document.createElement('br'));
      count++;
    });
    const mapElement = document.getElementById(this.options.mapDivId);
    outerdiv.style.width = mapElement.offsetWidth + 'px';
    outerdiv.appendChild(formElement);
    const mapControlElement = document.getElementById('select-map-control');
    mapControlElement.appendChild(outerdiv);
    const fitToWidth =  formElement.offsetWidth;
    outerdiv.style.width = fitToWidth + 'px';
    this.mapPopup = outerdiv;
  }

  handleSelectMapLayer() {
    let selectedLayer = 0;
    const radios = document.getElementsByName('layer');
    for (let i = 0; i < radios.length; i++) {
      if (radios[i].checked == true) {
        selectedLayer = radios[i].value;
        break;
      }
    }
    this.hide();
    this.mapLayerManager.changeLayer(selectedLayer);
  }

  hide() {
    const mapControlElement = document.getElementById('select-map-control');
    const popup = document.getElementById('map-layer-list-container');
    if (popup)
      mapControlElement.removeChild(popup);
  }
}

class ExitMapLayerControl extends Control {

  constructor(mapLayerManager, opt_options) {
    const options = opt_options || {};

    const button = document.createElement('button');
    button.setAttribute('accesskey', 'x');
    button.className = 'map-exit-control';

    const element = document.createElement('div');
    element.id = 'exit-map-control';
    element.className = 'exit-map-layer ol-unselectable ol-control';
    element.appendChild(button);

    super({
      element: element,
      target: options.target,
    });
    self = this;
    this.mapLayerManager = mapLayerManager;

    button.addEventListener('mouseover', this.handleSelectModal.bind(this), false);
    button.addEventListener('click',
                            function() {
                              new MapExitModal(self.mapLayerManager).handleExit();
                            }, false);
    element.addEventListener('mouseleave', this.handleHideModal.bind(this), false);
  }

  handleSelectModal() {
    this.modal = new MapExitModal(this.mapLayerManager);
  }

  handleHideModal() {
    if (this.modal) {
      this.modal.hide();
    }
  }

}

class MapExitModal {

  constructor(mapLayerManager) {
    this.mapLayerManager = mapLayerManager;
    const outerdiv = document.createElement('div');
    outerdiv.id = 'exit-map-container';
    const formElement = document.createElement('div');
    formElement.id = 'exit-map-form';
    formElement.className = 'exit-map-modal';

    const button = document.createElement('button');
    button.innerHTML = this.mapLayerManager.options.exitMessage;
    button.style.width = '8em';
    button.style.height = '2em';
    button.className = 'map-exit-confirm';
    button.addEventListener('click', this.handleExit.bind(this), false);
    formElement.appendChild(button);

    const mapElement = document.getElementById(this.mapLayerManager.options.mapDivId);
    outerdiv.appendChild(formElement);
    const mapControlElement = document.getElementById('exit-map-control');
    mapControlElement.appendChild(outerdiv);
    const fitToWidth =  formElement.offsetWidth;
    outerdiv.style.width = fitToWidth + 'px';
    this.mapPopup = outerdiv;
  }

  handleExit() {
    this.hide();
    const itinerary_id = this.mapLayerManager.options.itinerary_id;
    const target = this.mapLayerManager.options.exitUrl;
    location.assign(target);
  }

  hide() {
    const mapControlElement = document.getElementById('exit-map-control');
    const popup = document.getElementById('exit-map-container');
    mapControlElement.removeChild(popup);
  }
}

class TripMap {

  constructor(providers, opt_options) {
    this.options = opt_options || {};
    this.options.mapDivId = this.options.mapDivId === undefined ? 'map' : this.options.mapDivId;
    this.options.trackArrowFrequency = this.options.trackArrowFrequency === undefined ? 25 : this.options.trackArrowFrequency;
    this.options.routeArrowFrequency = this.options.routeArrowFrequency === undefined ? 10 : this.options.routeArrowFrequency;
    this.options.showExitControl = this.options.showExitControl === undefined ? true : this.options.showExitControl;
    this.providers = providers;
    this.currentProviderIndex = 0;
    const self = this;
    const scaleControl = new ol.control.ScaleLine({
      units: 'metric',
      bar: true,
      steps: 4,
      text: true,
      minWidth: 64,
    });
    const controls = [scaleControl];
    if (providers.length > 1)
      controls.push(new SelectMapLayerControl(self, self.options));
    if (this.options.showExitControl)
      controls.push(new ExitMapLayerControl(self));
    this.map = new ol.Map({
      controls: defaultControls().extend(controls),
      target: self.options.mapDivId,
      layers: [
        this.createMapLayer(0),
      ],
      view: new View({
        // Using default projection of Spherical Mercator (EPSG:3857)
        projection: 'EPSG:3857',
        center: fromLonLat([0, 0]),
        zoom: 1,
        maxZoom: 20,
        padding: [10, 10, 70, 10],
      }),
    });
    this.fetchFeatures();
  }

  fetchFeatures() {
    const self = this;
    // console.debug('Fetching from:', self.options.url);
    const myHeaders = new Headers([
      ['Content-Type', 'application/json; charset=UTF-8']
    ]);
    const myRequest = new Request(
      self.options.url,
      {
        method: 'POST',
        body: JSON.stringify(self.options.pageInfo),
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
        // console.debug('Status: ', response.status);
        return response.json();
      })
      .then((data) => {
        self.handleUpdate(data);
      })
      .catch((error) => {
        console.error('Error fetching itinerary map data:', error);
      });
  } // fetchFeatures()

  styleFunction(feature) {
    const geometry = feature.getGeometry();
    const geometryType = feature.getGeometry().getType();
    const featureType = feature.get('type');
    const self = this;
    if (geometryType == 'Point') {
      const point = this.createPointImage();
      let name = feature.get('name');
      if (!name) {
        const id = feature.get('id');
        if (id)
          name = `ID: ${id}`;
      }
      return new Style({
        image: point,
        text: new TextStyle({
          text: name,
          font: '12px Calibri,sans-serif',
          offsetX: 0,
          offsetY: -18,
          fill: new Fill({
            color: 'black',
          }),
          // backgroundFill: new Fill({
          //   color: 'white',
          // }),
        }),
        padding: [20, 40, 20, 40],
      });
    }
    let featureColor = feature.get('html_color_code');
    const isTrack = featureType == 'track';
    const frequency = isTrack ? self.options.trackArrowFrequency : self.options.routeArrowFrequency;
    if (!featureColor)
      featureColor = isTrack ? 'red' : 'magenta';
    const width = isTrack ? 2 : 4;
    const styles = [
      // linestring
      new Style({
        stroke: new Stroke({
          color: featureColor,
          width: width,
        })
      })
    ];
    if (frequency > 0) {
      const arrowRadius = isTrack ? 5 : 8;
      if (geometryType == 'LineString') {
        let count = 0;
        geometry.forEachSegment(function(start, end) {
          self.renderSegmentArrows(start, end, count, frequency, styles, featureColor, arrowRadius);
          count++;
        });
      } else if (geometryType == 'MultiLineString') {
        geometry.getLineStrings().forEach(lineString => {
          let count = 0;
          lineString.forEachSegment(function(start, end) {
            self.renderSegmentArrows(start, end, count, frequency, styles, featureColor, arrowRadius);
            count++;
          });
        });
      }
    }
    return styles;
  }

  renderSegmentArrows(start, end, count, frequency, styles, featureColor, arrowRadius) {
    if (count % frequency == 0) {
      const dx = end[0] - start[0];
      const dy = end[1] - start[1];
      const rotation = Math.atan2(dy, dx);
      // arrows
      styles.push(new Style({
        geometry: new Point(ol.extent.getCenter([start[0], start[1], end[0], end[1]])),
        image: new RegularShape({
          fill: new Fill({color: featureColor}),
          points: 3,
          radius: arrowRadius,
          rotation: -rotation,
          rotateWithView: true,
          angle: Math.PI / 2 // rotate 90°
        })
      }));
    }
  }

  createPointImage() {
    const self = this;
    return new CircleStyle({
      radius: 10,
      fill: null,
      // fill: new Fill({color: 'red'}),
      stroke: new Stroke({color: 'blue', width: 4})
    });
  }

  // In OpenLayers v7.1.0, the OSM tile layer is same as XYZ, except for:
  // attributions default to OSM
  // attributionsCollapsible is false
  // crossOrigin = 'anonymous'
  // url defaults to the OSM tile server
  // maxZoom defaults to 19
  // opaque defaults to true
  createMapLayer(providerIndex) {
    const provider = this.providers[providerIndex];
    return new TileLayer({
      source: new XYZ({
        attributions: [
          provider.attributions,
        ],
        attributionsCollapsible: false,
        url: provider.url,
        crossOrigin: 'anonymous',
        minZoom: provider.min_zoom,
        maxZoom: provider.max_zoom,
        opaque: true,
      }),
    });
  }

  changeLayer(providerIndex) {
    this.currentProviderIndex = providerIndex;
    const newTileLayer = this.createMapLayer(providerIndex);
    this.map.setLayers([newTileLayer]);
    if (this.trackLayer)
      this.map.addLayer(this.trackLayer);
    if (this.routeLayer)
      this.map.addLayer(this.routeLayer);
    if (this.waypointLayer)
      this.map.addLayer(this.waypointLayer);
    if (this.locationLayer)
      this.map.addLayer(this.locationLayer);
  }

  setTrackLayer(trackLayer) {
    this.trackLayer = trackLayer;
  }

} // TripMap

class CreateFeatureControl extends Control {

  constructor(callback, opt_options) {
    const routeButton = document.createElement('button');
    routeButton.setAttribute('accesskey', 'r');
    routeButton.setAttribute('data-bs-original-title', 'Create route');
    routeButton.className = "map-route-control";

    const waypointButton = document.createElement('button');
    waypointButton.setAttribute('accesskey', 'w');
    waypointButton.setAttribute('data-bs-original-title', 'Create waypoint');
    waypointButton.className = "map-waypoint-control";

    const element = document.createElement('div');
    element.id = 'create-feature-control';
    element.className = 'create-feature-layer ol-unselectable ol-control';
    element.appendChild(routeButton);
    element.appendChild(waypointButton);

    super({
      element: element,
      target: opt_options.target,
    });
    const self = this;
    this.options = opt_options || {};
    this.eventHandler = callback;

    routeButton.addEventListener('click', function() {
      // Abort any existing edit
      self.eventHandler(new CustomEvent('abort'));
      const options = Object.assign({}, self.options);
      options.div = 'create-feature-control';
      self.optionButtons = new ModifyFeatureOptionButtons(
        self.optionButtonsEventHandler.bind(self), options);
      self.eventHandler(new CustomEvent('startroute'));
    }, false);
    waypointButton.addEventListener('click', function() {
      // Abort any existing edit
      self.eventHandler(new CustomEvent('abort'));
      const options = Object.assign({}, self.options);
      options.div = 'create-feature-control';
      options.finish = false;
      options.undo = false;
      options.top = '1.375em';
      self.optionButtons = new ModifyFeatureOptionButtons(
        self.optionButtonsEventHandler.bind(self), options);
      self.eventHandler(new CustomEvent('createwaypoint'));
    }, false);
  }

  optionButtonsEventHandler(event) {
    this.eventHandler(event);
  }

  hideOptionButtons() {
    if (this.optionButtons)
      this.optionButtons.hide();
  }

}

class ModifyFeatureOptionButtons {

  constructor(callback, opt_options) {
    this.options = opt_options || {};
    this.eventHandler = callback;
    this.options = opt_options || {};
    const outerElement = document.createElement('ul');
    outerElement.id = 'create-feature-options';
    if (this.options.top)
      outerElement.style.top = this.options.top;
    outerElement.className = 'modify-feature-option-container';
    if (this.options.finish !== false) {
      const finishElement = document.createElement('li');
      const finishLink = document.createElement('a');
      finishLink.addEventListener('click', this.handleFinishCreateRoute.bind(this), false);
      finishLink.innerHTML = 'Finish';
      finishLink.setAttribute('href', '#');
      finishLink.setAttribute('title', 'Finish creating route');
      finishElement.appendChild(finishLink);
      outerElement.appendChild(finishElement);
    }
    if (this.options.save === true) {
      const saveElement = document.createElement('li');
      const saveLink = document.createElement('a');
      saveLink.addEventListener('click', this.handleSaveCreateRoute.bind(this), false);
      saveLink.innerHTML = 'Save';
      saveLink.setAttribute('href', '#');
      saveLink.setAttribute('title', 'Save changes');
      saveElement.appendChild(saveLink);
      outerElement.appendChild(saveElement);
    }
    if (this.options.undo !== false) {
      const undoElement = document.createElement('li');
      const undoLink = document.createElement('a');
      undoLink.addEventListener('click', this.handleUndoCreateRoute.bind(this), false);
      undoLink.innerHTML = 'Undo';
      undoLink.setAttribute('href', '#');
      undoLink.setAttribute('title', 'Delete last point');
      undoElement.appendChild(undoLink);
      outerElement.appendChild(undoElement);
    }
    const cancelElement = document.createElement('li');
    const cancelLink = document.createElement('a');
    cancelLink.addEventListener('click', this.handleCancelCreateRoute.bind(this), false);
    cancelLink.innerHTML = 'Cancel';
    cancelLink.setAttribute('href', '#');
    cancelLink.setAttribute('title', 'Cancel creating feature');
    cancelElement.appendChild(cancelLink);
    outerElement.appendChild(cancelElement);

    const controlDiv = document.getElementById(this.options.div);
    controlDiv.appendChild(outerElement);
  }

  handleFinishCreateRoute() {
    this.eventHandler(new CustomEvent('routecreated'));
    this.hide();
  }

  handleSaveCreateRoute() {
    this.eventHandler(new CustomEvent('save'));
    this.hide();
  }

  handleCancelCreateRoute() {
    this.eventHandler(new CustomEvent('abort'));
    this.hide();
  }

  handleUndoCreateRoute() {
    this.eventHandler(new CustomEvent('undo'));
  }

  hide() {
    const optionsElement = document.getElementById('create-feature-options');
    if (optionsElement)
      optionsElement.remove();
  }
}

class ModifyFeatureControl extends Control {

  constructor(callback, opt_options) {
    const options = opt_options || {};

    const editButton = document.createElement('button');
    editButton.setAttribute('accesskey', 'e');
    editButton.setAttribute('data-bs-original-title', 'Edit features');
    editButton.className = 'map-edit-control';
    const deleteButton = document.createElement('button');
    deleteButton.setAttribute('accesskey', 'd');
    deleteButton.setAttribute('data-bs-original-title', 'Delete features');
    deleteButton.className = 'map-delete-control';

    const element = document.createElement('div');
    element.id = 'modify-feature-control';
    element.className = 'modify-feature-layer ol-unselectable ol-control';
    element.appendChild(editButton);
    element.appendChild(deleteButton);

    super({
      element: element,
      target: options.target,
    });
    this.options = options;
    this.eventHandler = callback;
    const self = this;
    editButton.addEventListener('click', function() {
      self.eventHandler(new CustomEvent('abort'));
      const options = Object.assign({}, self.options);
      options.div = 'modify-feature-control';
      options.save = true;
      options.finish = false;
      options.undo = false;
      options.top = '0em';
      self.optionButtons = new ModifyFeatureOptionButtons(
        self.optionButtonsEventHandler.bind(self),
        options);
      self.eventHandler(new CustomEvent('edit'));
    }, false);
    deleteButton.addEventListener('click', function() {
      self.eventHandler(new CustomEvent('abort'));
      const options = Object.assign({}, self.options);
      options.div = 'modify-feature-control';
      options.save = true;
      options.finish = false;
      options.undo = false;
      options.top = '1.375em';
      self.optionButtons = new ModifyFeatureOptionButtons(
        self.optionButtonsEventHandler.bind(self),
        options);
      self.eventHandler(new CustomEvent('delete-select'));
    }, false);
  }

  optionButtonsEventHandler(event) {
    this.eventHandler(event);
  }

  hideOptionButtons() {
    if (this.optionButtons)
      this.optionButtons.hide();
  }

}

class LocationSharingControl extends Control {

  constructor(callback, opt_options) {
    const options = opt_options || {};
    const shareButton = document.createElement('button');
    shareButton.setAttribute('accesskey', 'o');
    shareButton.setAttribute('data-bs-original-title', 'Map options');
    shareButton.className = 'map-live-control';

    const element = document.createElement('div');
    element.id = 'location-sharing-control';
    element.className = 'location-sharing-layer ol-unselectable ol-control';
    element.appendChild(shareButton);

    super({
      element: element,
      target: options.target,
    });
    const self = this;
    this.shareButton = shareButton;
    this.options = options;
    this.options.stop = true;
    this.options.mapDivId = this.options.mapDivId === undefined ? 'map' : this.options.mapDivId;
    const start = new Date();
    start.setHours(0);
    start.setMinutes(0);
    start.setSeconds(0);
    start.setMilliseconds(0);
    this.formData = {
      start: start,
      nicknames: new Map(),
      maxHdop: 0,
      trackMe: false,
      refreshInterval: 60,
      trackingInterval: 60,
    };
    const pathColors = this.options.pathColors;
    let count = 0;
    this.options.nicknames.forEach(function(nickname) {
      let value = { selected: false };
      if (pathColors && pathColors.length > 0) {
        value.pathColor = pathColors[count];
        if (++count >= pathColors.length) {
          count = 0;
        }
      }
      self.formData.nicknames.set(nickname, value);
    });
    this.eventHandler = callback;
    this.shareButton.addEventListener('click', this.showForm.bind(this), false);
    // element.addEventListener('mouseleave', this.hideForm.bind(this), false);
  }

  setStopState(state) {
    this.options.stop = state;
  }

  showForm(event) {
    const self = this;
    if (!this.form) {
      this.form = new LocationSharingModal(
        (event) => {
          self.eventHandler(event);
          self.hideForm();
        },
        self.formData,
        self.options);
    }
  }

  hideForm() {
    if (this.form) {
      this.form.hide();
      this.form = undefined;
    }
  }

}

class LocationSharingModal {

  constructor(callback, formData, opt_options) {
    this.options = opt_options || {};
    this.formData = formData;
    this.eventHandler = callback;
    const outerdiv = document.createElement('div');
    outerdiv.id = 'location-sharing-outer-div';

    const formDiv = document.createElement('div');
    formDiv.className = 'location-sharing-modal py-2';
    const formElement = document.createElement('form');
    formDiv.appendChild(formElement);
    const pathColors = this.options.pathColors;
    let count = 0;
    const self = this;
    this.formData.nicknames.forEach(function(value, nickname, map) {
      const htmlNickname = nickname;
      const nicknameId = 'nickname-' + count;
      sanitize(htmlNickname);
      const label = document.createElement('label');
      label.htmlFor = nicknameId;
      label.innerHTML = htmlNickname;
      label.className = 'location-sharing-nickname';
      label.style.color = value.pathColor.html_code;
      const input = document.createElement('input');
      input.id = nicknameId;
      input.type = 'checkbox';
      input.name = htmlNickname;
      input.className = 'location-sharing-nickname';
      input.checked = value.selected;
      // input.addEventListener('click', self.handleNicknameSelect.bind(self), false);
      formElement.appendChild(input);
      formElement.appendChild(label);
      formElement.appendChild(document.createElement('br'));
      count++;
    });
    const startLabel = document.createElement('label');
    startLabel.htmlFor = 'start';
    startLabel.innerHTML = 'Starting from';
    startLabel.className = 'me-2 mb-2';
    const startInput = document.createElement('input');
    startInput.type = 'datetime-local';
    startInput.name = 'start';
    startInput.id = 'input-start';
    startInput.className = 'mb-2';
    const startTime = new Date(this.formData.start);
    startTime.setMinutes(startTime.getMinutes() - startTime.getTimezoneOffset());
    const s = startTime.toISOString();
    const dateStr = s.substring(0, 10) + s.substring(10, 16);
    startInput.value = dateStr;
    formElement.appendChild(startLabel);
    formElement.appendChild(startInput);
    formElement.appendChild(document.createElement('br'));
    const hdopLabel = document.createElement('label');
    hdopLabel.htmlFor = 'hdop-input';
    hdopLabel.innerHTML = 'Max hdop&nbsp;';
    const hdopInput = document.createElement('input');
    hdopInput.id = 'input-max-hdop';
    hdopInput.type = 'number';
    hdopInput.name = 'max-hdop';
    hdopInput.value = formData.maxHdop == 0 ? '' : formData.maxHdop;
    hdopInput.setAttribute('min', 0);
    hdopInput.setAttribute('max', 9999);
    formElement.appendChild(hdopLabel);
    formElement.appendChild(hdopInput);
    formElement.appendChild(document.createElement('br'));

    const refreshIntervalLabel = document.createElement('label');
    refreshIntervalLabel.htmlFor = 'refresh-interval-input';
    refreshIntervalLabel.innerHTML = 'Refresh interval (seconds)&nbsp;';
    refreshIntervalLabel.className = 'mb-2';
    const refreshIntervalElement = document.createElement('input');
    refreshIntervalElement.id = 'refresh-interval-input';
    refreshIntervalElement.type = 'number';
    refreshIntervalElement.name = 'refresh-interval';
    refreshIntervalElement.className = 'mb-2';
    refreshIntervalElement.value = formData.refreshInterval;
    refreshIntervalElement.setAttribute('min', 10);
    refreshIntervalElement.setAttribute('max', 86400);
    refreshIntervalElement.setAttribute('step', 1);
    formElement.appendChild(refreshIntervalLabel);
    formElement.appendChild(refreshIntervalElement);
    formElement.appendChild(document.createElement('br'));
    formElement.appendChild(document.createElement('hr'));

    const trackMeLabel = document.createElement('label');
    trackMeLabel.htmlFor = 'track-me-checkbox';
    trackMeLabel.innerHTML = 'Track & record yourself&nbsp;';
    trackMeLabel.className = 'mb-2';
    const trackMeElement = document.createElement('input');
    trackMeElement.id = 'track-me-checkbox';
    trackMeElement.type = 'checkbox';
    trackMeElement.name = 'track-me';
    trackMeElement.className = 'mb-2';
    trackMeElement.checked = (formData.trackMe === true);
    formElement.appendChild(trackMeLabel);
    formElement.appendChild(trackMeElement);
    formElement.appendChild(document.createElement('br'));

    const trackingIntervalLabel = document.createElement('label');
    trackingIntervalLabel.htmlFor = 'tracking-interval-input';
    trackingIntervalLabel.innerHTML = 'Tracking interval (seconds)&nbsp;';
    trackingIntervalLabel.className = 'mb-2';
    const trackingIntervalElement = document.createElement('input');
    trackingIntervalElement.id = 'tracking-interval-input';
    trackingIntervalElement.type = 'number';
    trackingIntervalElement.name = 'tracking-interval';
    trackingIntervalElement.className = 'mb-2';
    trackingIntervalElement.value = formData.trackingInterval;
    trackingIntervalElement.setAttribute('min', 10);
    trackingIntervalElement.setAttribute('max', 86400);
    trackingIntervalElement.setAttribute('step', 1);
    formElement.appendChild(trackingIntervalLabel);
    formElement.appendChild(trackingIntervalElement);
    formElement.appendChild(document.createElement('br'));

    const buttonDiv = document.createElement('div');
    buttonDiv.className = 'my-2';
    formElement.appendChild(buttonDiv);
    const applyButton = document.createElement('button');
    applyButton.innerHTML = this.options.stop ? 'Start' : 'Apply';
    applyButton.type = 'button';
    applyButton.className = 'live-map-form btn btn-success me-2';
    applyButton.setAttribute('accesskey', 's');
    buttonDiv.appendChild(applyButton);
    const stopButton = document.createElement('button');
    stopButton.id = 'btn-stop';
    stopButton.innerHTML = 'Stop';
    stopButton.type = 'button';
    stopButton.className = 'live-map-form btn ' + (this.options.stop ? 'btn-secondary' : 'btn-primary') + ' me-2';
    stopButton.setAttribute('accesskey', 'p');
    buttonDiv.appendChild(stopButton);
    const cancelButton = document.createElement('button');
    cancelButton.innerHTML = 'Cancel';
    cancelButton.type = 'button';
    cancelButton.className = 'live-map-form btn btn-danger';
    cancelButton.setAttribute('accesskey', 'c');
    buttonDiv.appendChild(cancelButton);
    // const divElement = document.getElementById(this.options.div);
    // outerdiv.style.width = divElement.offsetWidth + 'px';
    outerdiv.appendChild(formDiv);
    // options.div = this.options.mapDivId;
    // const controlElement = document.getElementById('location-sharing-control');
    const controlElement = document.getElementById(this.options.mapDivId);
    controlElement.appendChild(outerdiv);
    const fitToWidth = formDiv.offsetWidth;
    formDiv.style.width = fitToWidth + 'px';
    outerdiv.style.width = fitToWidth + 'px';
    applyButton.addEventListener('click', self.handleApply.bind(self), false);
    stopButton.addEventListener('click', self.handleStop.bind(self), false);
    cancelButton.addEventListener('click', self.handleCancel.bind(self), false);
  }

  updateFormData() {
    this.formData.nicknames.forEach(function(value, nickname, map) {
      value.selected = false;
      map.set(nickname, value);
    });
    let selectedLayer = 0;
    const checkboxes = document.getElementsByClassName('location-sharing-nickname');
    for (let i = 0; i < checkboxes.length; i++) {
      if (checkboxes[i].type == 'checkbox' && checkboxes[i].checked == true) {
        const name = checkboxes[i].name;
        let entry = this.formData.nicknames.get(name);
        entry.selected = checkboxes[i].value == 'on';
        this.formData.nicknames.set(name, entry);
      }
    }
    const startInput = document.getElementById('input-start');
    const dt = new Date(startInput.value);
    if (!isNaN(dt)) {
      this.formData.start = dt;
    }
    const hdopInput = document.getElementById('input-max-hdop');
    this.formData.maxHdop = hdopInput.value;

    this.formData.trackMe = document.getElementById('track-me-checkbox').checked;
    this.formData.refreshInterval = document.getElementById('refresh-interval-input').value;
    this.formData.trackingInterval = document.getElementById('tracking-interval-input').value;
  }

  handleApply() {
    const self = this;
    this.updateFormData();
    this.eventHandler(new CustomEvent('start', { detail: self.formData} ));
    // this.hide();
  }

  handleStop() {
    const self = this;
    this.updateFormData();
    this.eventHandler(new CustomEvent('stop', { detail: self.formData} ));
    // this.hide();
  }

  handleCancel() {
    this.eventHandler(new CustomEvent('cancel'));
    // this.hide();
  }

  handleNicknameSelect() {
  }

  hide() {
    const mapDiv = document.getElementById(this.options.mapDivId);
    const popup = document.getElementById('location-sharing-outer-div');
    mapDiv.removeChild(popup);
  }

}
