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
const meters_per_mile = 1609.344;
const meters_per_foot = 0.0254 * 12;
const fromLonLat = ol.proj.fromLonLat;
const CircleStyle = ol.style.Circle;
const Control = ol.control.Control;
const defaultControls = ol.control.defaults.defaults;
const Feature = ol.Feature;
const Fill = ol.style.Fill;
const GeoJSON = ol.format.GeoJSON;
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
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;
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

    button.addEventListener('mouseover', this.handleSelectMapLayer.bind(this), false);
    element.addEventListener('mouseleave', this.handleHideModal.bind(this), false);
  }

  handleSelectMapLayer() {
    this.modal = new SelectMapModal(this.mapLayerManager);
  }

  handleHideModal() {
    if (this.modal) {
      this.modal.hide();
    }
  }
}

class SelectMapModal {

  constructor(mapLayerManager) {
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
    const mapElement = document.getElementById('map');
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

    const element = document.createElement('div');
    element.id = 'exit-map-control';
    element.className = 'exit-map-layer ol-unselectable ol-control';
    element.appendChild(button);

    super({
      element: element,
      target: options.target,
    });
    this.mapLayerManager = mapLayerManager;

    button.addEventListener('mouseover', this.handleSelectModal.bind(this), false);
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
    button.innerHTML = click_to_exit_text;
    button.style.width = '8em';
    button.style.height = '4em';
    button.className = 'map-exit-confirm';
    button.addEventListener('click', this.handleExit.bind(this), false);
    formElement.appendChild(button);

    const mapElement = document.getElementById('map');
    outerdiv.appendChild(formElement);
    const mapControlElement = document.getElementById('exit-map-control');
    mapControlElement.appendChild(outerdiv);
    const fitToWidth =  formElement.offsetWidth;
    outerdiv.style.width = fitToWidth + 'px';
    this.mapPopup = outerdiv;
  }

  handleExit() {
    this.hide();
    const pageInfo = JSON.parse(pageInfoJSON);
    const itinerary_id = pageInfo.itinerary_id;
    const target = `${server_prefix}/itinerary?id=${itinerary_id}&active-tab=features`;
    location.assign(target);
  }

  hide() {
    const mapControlElement = document.getElementById('exit-map-control');
    const popup = document.getElementById('exit-map-container');
    mapControlElement.removeChild(popup);
  }
}

class ItineraryMap {

  constructor(providers, url) {
    this.providers = providers;
    this.url = url;
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
      controls.push(new SelectMapLayerControl(self));
    controls.push(new ExitMapLayerControl(self));
    this.map = new Map({
      controls: defaultControls().extend(controls),
      target: 'map',
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
    // console.debug('Fetching from:', this.url);
    const myHeaders = new Headers([
      ['Content-Type', 'application/json; charset=UTF-8']
    ]);
    const myRequest = new Request(
      self.url,
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

  handleUpdate(data) {
    const self = this;
    // console.debug('data:', data);
    if (data.tracks.features.length > 0) {
      const trackFeatures = new GeoJSON().readFeatures(data.tracks);
      trackFeatures.forEach(function(feature) {
        // Using EPSG:4326 for WGS 84 from the GeoJSON format
        const geometry = feature.getGeometry();
        geometry.transform('EPSG:4326', 'EPSG:3857');
        // feature.setGeometry(geometry.simplify(90));
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

  styleFunction(feature) {
    const geometry = feature.getGeometry();
    const type = feature.getGeometry().getType();
    const self = this;
    if (type == 'Point') {
      const point = this.createPointImage();
      let name = feature.get('name');
      if (!name) {
        const id = feature.get('id');
        if (id)
          name = `WPT: {id}`;
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
    const featureType = feature.get('type');
    if (!featureColor)
      featureColor = featureType == 'track' ? 'red' : 'magenta';
    const width = feature.get('type') == 'track' ? 2 : 5;
    const styles = [
      // linestring
      new Style({
        stroke: new Stroke({
          color: featureColor,
          width: width,
        })
      })
    ];
    const arrowRadius = featureType == 'track' ? 5 : 10;
    geometry.forEachSegment(function(start, end) {
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
    });
    return styles;
  }

  createPointImage() {
    const self = this;
    return new CircleStyle({
      radius: 10,
      fill: null,
      stroke: new Stroke({color: 'blue', width: 2})
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
    if (this.trackLayer) {
      this.map.addLayer(this.trackLayer);
    }
  }

  setTrackLayer(trackLayer) {
    this.trackLayer = trackLayer;
  }

} // ItineraryMap

const itineraryMap = new ItineraryMap(providers,
                                      server_prefix +
                                      '/rest/itinerary/features');
