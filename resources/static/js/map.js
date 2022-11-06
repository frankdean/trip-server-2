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
const TileLayer = ol.layer.Tile;
const VectorLayer = ol.layer.Vector;
const VectorSource = ol.source.Vector;
const View = ol.View;
const XYZ = ol.source.XYZ;

class TrackLayerManager {

  constructor(mapLayerManager, data, options) {
    this.mapLayerManager = mapLayerManager;
    this.options = options || {color: 'red'};
    if (!data.geojsonObject) {
      return;
    }
    let length = 0;
    const trackFeatures = new GeoJSON().readFeatures(data.geojsonObject);
    trackFeatures.forEach(function(feature) {
      // Using EPSG:4326 for WGS 84 from the GeoJSON format
      const geometry = feature.getGeometry();
      geometry.transform('EPSG:4326', 'EPSG:3857');
      length += ol.sphere.getLength(geometry);
    });

    const mapDiv = document.getElementById('map');
    const infoDiv = document.getElementById('track-info');
    const footerDiv = document.getElementById('footer');

    let metric = `${(length / 1000).toFixed(2)}&nbsp;km`;
    if (data.ascent)
      metric += `&nbsp;↗${data.ascent}&nbsp;m`;
    if (data.descent)
      metric += `&nbsp;↘${data.descent}&nbsp;m`;
    if (data.maxHeight  && data.minHeight) {
      metric += `&nbsp; ${data.maxHeight}⇅${data.minHeight}&nbsp;m`;
    }

    let imperial = `${(length / meters_per_mile).toFixed(2)}&nbsp;mi`;
    if (data.ascent)
      imperial += `&nbsp;↗${(data.ascent / meters_per_foot).toFixed(0)}&nbsp;ft`;
    if (data.descent)
      imperial += `&nbsp;↘${(data.descent / meters_per_foot).toFixed(0)}&nbsp;ft`;
    if (data.maxHeight  && data.minHeight) {
      imperial += `&nbsp; ${(data.maxHeight / meters_per_foot).toFixed(0)}⇅${(data.minHeight / meters_per_foot).toFixed(0)}&nbsp;ft`;
    }

    infoDiv.style.bottom = footerDiv.offsetHeight + 'px';
    infoDiv.innerHTML = `<p>${metric}</br>${imperial}`;
    infoDiv.style.background = '#e6e6e6'; //'rgb(233, 237, 242)'; // 'rgb(248, 249, 250)';
    mapDiv.style.bottom = `${infoDiv.offsetHeight + footerDiv.offsetHeight}px`;
    const trackSource = new VectorSource({
      features: trackFeatures,
    });

    const self = this;
    this.trackLayer = new VectorLayer({
      source: trackSource,
      style: this.styleFunction.bind(self),
    });

    this.mapLayerManager.map.getView().fit(this.trackLayer.getSource().getExtent());
    this.mapLayerManager.map.addLayer(this.trackLayer);

    if (data.most_recent) {
      const container = document.getElementById('popup');
      const content = document.getElementById('popup-content');
      const closer = document.getElementById('popup-closer');

      const overlayPosition = fromLonLat(data.most_recent.position);

      trackSource.addFeatures([
        new ol.Feature({
          geometry: new Point(overlayPosition)
        })
      ]);

      const overlay = new ol.Overlay({
        element: container,
        autoPan: true,
        autoPanAnimation: {
          duration: 250
        }
      });
      this.mapLayerManager.map.addOverlay(overlay);
      closer.onclick = function() {
        overlay.setPosition(undefined);
        closer.blur();
        return false;
      };
      let markerContent;
      if (data.most_recent.note) {
        markerContent = `${data.most_recent.time}</br>${data.most_recent.note}`;
      } else {
        markerContent = data.most_recent.time;
      }
      content.innerHTML = markerContent;
      overlay.setPosition(overlayPosition);
      const self = this;
      this.mapLayerManager.map.on('singleclick', function(event) {
        if (self.mapLayerManager.map.hasFeatureAtPixel(event.pixel) === true) {
          // const coordinate = event.coordinate;
          content.innerHTML = markerContent;
          overlay.setPosition(overlayPosition);
        } else {
          overlay.setPosition(undefined);
          closer.blur();
        }
      });
      setTimeout(function() {
        overlay.panIntoView({
          autoPanAnimation: {
            duration: 250
          },
          margin: 20,
        });
      }, 200);
    }

    if (data.message) {
      setTimeout(function() {
        alert(data.message);
      }, 2000);
    }
  }

  createPointImage() {
    const self = this;
    return new CircleStyle({
      radius: 5,
      fill: null,
      stroke: new Stroke({color: self.options.color, width: 4})
    });
  }

  styleFunction(feature) {
    const geometry = feature.getGeometry();
    const type = feature.getGeometry().getType();
    const point = this.createPointImage();
    const self = this;
    if (type == 'Point') {
      return new Style({
        image: point,
      });
    }
    const styles = [
      // linestring
      new Style({
        stroke: new Stroke({
          color: self.options.color,
          width: 2,
        })
      })
    ];
    geometry.forEachSegment(function(start, end) {
      const dx = end[0] - start[0];
      const dy = end[1] - start[1];
      const rotation = Math.atan2(dy, dx);
      // arrows
      styles.push(new Style({
        geometry: new Point(ol.extent.getCenter([start[0], start[1], end[0], end[1]])),
        image: new RegularShape({
          fill: new Fill({color: self.options.color}),
          points: 3,
          radius: 5,
          rotation: -rotation,
          rotateWithView: true,
          angle: Math.PI / 2 // rotate 90°
        })
      }));
    });
    return styles;
  }

}

class MapLayerManager {

  constructor(providers) {
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
      controls.push(new SelectMapLayerControl(self));
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
    const newLayer = this.createMapLayer(providerIndex);
    this.map.setLayers([newLayer]);
    if (this.trackLayer) {
      this.map.addLayer(this.trackLayer);
    }
  }

  setTrackLayer(trackLayer) {
    this.trackLayer = trackLayer;
    this.changeLayer(this.currentProviderIndex);
  }
}

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

class MapPointViewer {
  constructor(mapLayerManager, lng, lat) {
    // this.mapLayerManager = mapLayerManager;
    // this.lng = lng;
    // this.lat = lat;
    const overlayPosition = fromLonLat([lng, lat]);
    let trackSource = new VectorSource({
      features: [new Feature({
        geometry: new Point(overlayPosition),
      })],
    });
    let pointStyle = new Style({
      image: new CircleStyle({
        radius: 5,
        fill: new Fill({
          color: 'red'
        }),
        stroke: new Stroke({color: 'red', width: 4})
      }),
    });
    let trackLayer = new VectorLayer({
      source: trackSource,
      style: pointStyle,
    });
    mapLayerManager.setTrackLayer(trackLayer);
    // mapLayerManager.map.getView().fit(trackLayer.getSource().getExtent());
    let view = mapLayerManager.map.getView();
    view.setCenter(overlayPosition);
    view.setZoom(17);
    const container = document.getElementById('popup');
    const content = document.getElementById('popup-content');
    const closer = document.getElementById('popup-closer');
    const overlay = new ol.Overlay({
      element: container,
      autoPan: true,
      autoPanAnimation: {
        duration: 250
      }
    });
    mapLayerManager.map.addOverlay(overlay);
    closer.onclick = function() {
      overlay.setPosition(undefined);
      closer.blur();
      return false;
    };
    let markerContent = `${lat},${lng}`;
    content.innerHTML = markerContent;
    overlay.setPosition(overlayPosition);
    mapLayerManager.map.on('singleclick', function(event) {
      if (mapLayerManager.map.hasFeatureAtPixel(event.pixel) === true) {
        content.innerHTML = markerContent;
        overlay.setPosition(overlayPosition);
      } else {
        overlay.setPosition(undefined);
        closer.blur();
      }
    });
  }
}

const query_params = (new URL(document.location)).searchParams;

const lat = query_params.get('lat');
const lng = query_params.get('lng');

if (lat && lng) {
  new MapPointViewer(new MapLayerManager(providers), lng, lat);
} else {
  const myHeaders = new Headers();
  const myRequest = new Request(
    server_prefix + '/rest/locations?' +
      new URLSearchParams({
        nickname: query_params.get('nickname'),
        from: query_params.get('from'),
        to: query_params.get('to'),
        max_hdop: query_params.get('max_hdop'),
        notes_only_flag: query_params.get('notes_only_flag'),
        order: 'ASC',
        offset: -1,
        page_size: -1,
      }),
    {
      method: 'GET',
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
      // console.log('Status:', response.status);
      return response.json();
    })
    .then((data) => {
      // console.log(data);
      let mapLayerManager = new MapLayerManager(providers);
      const trackLayerManager = new TrackLayerManager(mapLayerManager, data);
      mapLayerManager.setTrackLayer(trackLayerManager.trackLayer);
    })
    .catch((error) => {
      console.error('Error fetching from Trip:', error);
    });
}
