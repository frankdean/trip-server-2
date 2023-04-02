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

export { TripMap, CreateFeatureControl, ModifyFeatureControl };

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
const Map = ol.Map;
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

class SelectMapLayerControl extends Control {

  constructor(mapLayerManager, opt_options) {
    const options = opt_options || {};

    const button = document.createElement('button');
    button.innerHTML = 'M';

    const element = document.createElement('div');
    element.id = 'select-map-control';
    element.className = 'select-map-layer ol-unselectable ol-control';
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
      label.innerHTML = p.name;
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
    mapControlElement.removeChild(popup);
  }
}

class ExitMapLayerControl extends Control {

  constructor(mapLayerManager, opt_options) {
    const options = opt_options || {};

    const button = document.createElement('button');
    button.innerHTML = 'X';
    button.setAttribute('accesskey', 'x');

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
    this.map = new Map({
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
        body: pageInfoJSON,
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
  }

  setTrackLayer(trackLayer) {
    this.trackLayer = trackLayer;
  }

} // TripMap

class CreateFeatureControl extends Control {

  constructor(callback, opt_options) {
    const routeButton = document.createElement('button');
    routeButton.innerHTML = 'R';
    routeButton.setAttribute('accesskey', 'r');

    const waypointButton = document.createElement('button');
    waypointButton.innerHTML = 'W';
    waypointButton.setAttribute('accesskey', 'w');

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
    editButton.innerHTML = 'E';
    editButton.setAttribute('accesskey', 'e');
    const deleteButton = document.createElement('button');
    deleteButton.innerHTML = 'D';
    deleteButton.setAttribute('accesskey', 'd');

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
