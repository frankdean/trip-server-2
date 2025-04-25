// -*- mode: js2; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=js norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2025 Frank Dean <frank.dean@fdsd.co.uk>

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU Affero General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
    for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

export { parseLocationText, convertToFormat };

// Most of this code is copied from Trip Server v1 `trip-web-client/app/js/utils-factory.js`.
// It is deliberately indented to match the indent in that file to make comparison easier.
// Note: the coordinate order of function parameters is lat, lng not x, y.

var $log = console;

       var _singleZeroPad = function(v) {
         return (v < 10 ? '0' + v : v);
       };

       var _tokenizeFormatString = function(v) {
         var r = [];
         for (var i = 0, n = v.length; i < n; ++i) {
           if (v[i] === '%' && i < n-1) {
             r.push(v[i] + v[++i]);
           } else {
             r.push(v[i]);
           }
         }
         return r;
       };

       var validateCoordinate = function(lat, lng) {
         return (lat >= -90 && lat <= 90 && lng >= -180 && lng <= 180);
       };

       var validateCoordinates = function(coords) {
         var retval = true;
         if (Array.isArray(coords)) {
           coords.forEach(function(v, k, a) {
             // $log.debug(k, ' => ', v);
             if (!validateCoordinate(v.lat, v.lng)) {
               $log.warn('Invalid lat/lng', v.lat, v.lng);
               retval = false;
             }
           });
         } else {
           $log.error('coords is not an array');
           retval = false;
         }
         return retval;
       };

       var formatCoordinates = function(v, format, latOrLng) {
         var r = '',
             dms,
             fsa = _tokenizeFormatString(format);
         if (fsa.indexOf('%s') >= 0 || fsa.indexOf('%S') >= 0) {
           dms = _degreesToDMS(v);
         } else if (format.includes('%m') || format.includes('%M')) {
           dms = _degreesToDM(v);
         } else {
           dms = {deg: Math.abs(v)};
         }
         fsa.forEach(function(e) {
           switch(e) {
           case '%%':
             r += '%';
             break;
           case '%d':
             r += dms.deg;
             break;
           case '%D':
             r += _singleZeroPad(dms.deg);
             break;
           case '%m':
             r += dms.min;
             break;
           case '%M':
             r += _singleZeroPad(dms.min);
             break;
           case '%s':
             r += dms.sec;
             break;
           case '%S':
             r += _singleZeroPad(dms.sec);
             break;
           case '%c':
             r += _cardinalSign(v, latOrLng);
             break;
           case '%i':
             r += (v < 0 ? '-' : '');
             break;
           case '%p':
             r += (v < 0 ? '-' : '+');
             break;
           default:
             r += e;
             break;
           }
         });
         return r;
       };
       var _degreesToDM = function(v) {
         var d, m;
         v = Math.abs(v);
         d = Math.floor(v);
         m = Math.round((v-d) * 60000000) / 1000000;
         return {deg: d, min: m};
       };
       var _degreesToDMS = function(v) {
         var dm, m, s;
         dm = _degreesToDM(v);
         m = Math.floor(dm.min);
         s = Math.round((dm.min - m) * 60000) / 1000;
         return {deg: dm.deg, min: m, sec: s};
       };
       var _cardinalSign = function(v, latOrLng) {
         var r;
         switch(latOrLng) {
         case 'lat':
           r = v < 0  ? 'S' : 'N';
           break;
         case 'lng':
           r = v < 0 ? 'W' : 'E';
           break;
         default:
           r = '';
           break;
         }
         return r;
       };

       var formatPosition = function(lat, lng, positionFormat) {
         var r;
         switch(positionFormat) {
         case 'lat,lng':
           r = lat + ',' + lng;
           break;
         case 'lng-lat':
           r = lng + ' ' + lat;
           break;
         case 'lng,lat':
           r = lng + ',' + lat;
           break;
         case undefined:
         case 'lat-lng':
           r = lat + ' ' + lng;
           break;
         default:
           r = lat + lng;
           break;
         }
         return r;
       };

       var bngMap = {
         HP: 'N42',
         HT: 'N31', HU: 'N41',
         HW: 'N10', HX: 'N20', HY: 'N30', HZ: 'N40',
         NA: '09', NB: '19', NC: '29', ND: '39',
         NF: '08', NG: '18', NH: '28', NJ: '38', NK:  '48',
         NL: '07', NM: '17', NN: '27', NO: '37',
         NR: '16', NS: '26', NT: '36', NU: '46',
         NW: '15', NX: '25', NY: '35', NZ: '45',
         SC: '24', SD: '34', SE: '44', TA: '54',
         SH: '23', SJ: '33', SK: '43', TF: '53', TG: '63',
         SM: '12', SN: '22', SO: '32', SP: '42', TL: '52', TM: '62',
         SR: '11', SS: '21', ST: '31', SU: '41', TQ: '51', TR: '61',
         SV: '00', SW: '10', SX: '20', SY: '30', SZ: '40', TV: '50'
       },
           irishGridMap = {
             A: '04', B: '14', C: '24', D: '34', E: '44',
             F: '03', G: '13', H: '23', J: '33', K: '43',
             L: '02', M: '12', N: '22', O: '32', P: '42',
             Q: '01', R: '11', S: '21', T: '31', U: '41',
             V: '00', W: '10', X: '20', Y: '30', Z: '40'
           };

       proj4.defs([
         // OSGB 1936 / British National Grid
         // https://epsg.io/27700
         ["EPSG:27700", "+proj=tmerc +lat_0=49 +lon_0=-2 +k=0.9996012717 +x_0=400000 +y_0=-100000 +ellps=airy +towgs84=446.448,-125.157,542.06,0.15,0.247,0.842,-20.489 +units=m +no_defs"],
         // TM65 / Irish Grid
         // https://epsg.io/29902
         ["EPSG:29902", "+proj=tmerc +lat_0=53.5 +lon_0=-8 +k=1.000035 +x_0=200000 +y_0=250000 +ellps=mod_airy +towgs84=482.5,-130.6,564.6,-1.042,-0.214,-0.631,8.15 +units=m +no_defs"],
         // IRENET95 / Irish Transverse Mercator
         // https://epsg.io/2157
         ["EPSG:2157", "+proj=tmerc +lat_0=53.5 +lon_0=-8 +k=0.99982 +x_0=600000 +y_0=750000 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs"]
       ]);

       var parseGeoLocation = function(text) {
         var lat = {}, lng = {}, found, coord, p4, x, y;
         var formats = [
           {name: 'plus+code', regex: /^(?:https?:\/\/.*\/)?([23456789CFGHJMPQRVWXcfghjmpqrvwx]{8}\+[23456789CFGHJMPQRVWXcfghjmpqrvwx]{2,3})$/},
           {name: 'osgb36', regex: /^(?:BNG|OSGB|OSGB36)?\s*(?:(HP|HT|HU|HW|HX|HY|HZ|NA|NB|NC|ND|NF|NG|NH|NJ|NK|NL|NM|NN|NO|NR|NS|NT|NU|NW|NX|NY|NZ|SC|SD|SE|TA|SH|SJ|SK|TF|TG|SM|SN|SO|SP|TL|TM|SM|SN|SO|SP|TL|TM|SR|SS|ST|SU|TQ|TR|SV|SW|SX|SY|SZ|TV)\s*(\d{3,5})\s*(\d{3,5})|(\d{6})[,\s]+(\d{6,7}))$/},
           {name: 'IrishGrid', regex: /^(?:ING|IG|TM65)?\s*(?:([A-HJ-Z]{1})\s*(\d{3,5})\s*(\d{3,5})|(?:ING|IG|TM65)\s*(\d{6})[,\s]+(\d{6}))$/},
           // Irish Transverse Mercator
           {name: 'ITM', regex: /^ITM\s*(?:E\s)?(\d{6}(?:\.\d{0,3})?)m?[,\s]+(?:N\s)?(\d{6}(?:\.\d{0,3})?)m?$/},
           {name: 'Google map', regex: /q=(?:loc:)?(-?[.\d]+),(-?[.\d]+)/, latd: 1, lngd: 2},
           {name: '+DMS', regex: /([NSns]{1})\s?([.\d]+)\s?(?:[-\s_\u00b0\u00baDd\u02da\u030a\u030a\u0325\u2070\u2218\u309a\u309c]{1}\s?(?:([.\d]+)\s?[-\s'_\u05f3\u02b9\u02bc\u02c8\u0301\u05f3\u2018\u2019\u201a\u201b\u2032\u2035\ua78c]{0,1}\s?(?:([.\d]+)\s?[-\s"_\u02ba\u030b\u030e\u05f4\u201c\u201d\u201e\u201f\u2033\u2036\u3003]{0,1}\s?)?)?)?[-_\s,]*([WEwe]{1})\s?([.\d]+)(?:[-\s_\u00b0\u00baDd\u02da\u030a\u030a\u0325\u2070\u2218\u309a\u309c]{1}\s?(?:([.\d]+)\s?[-\s'_\u05f3\u02b9\u02bc\u02c8\u0301\u05f3\u2018\u2019\u201a\u201b\u2032\u2035\ua78c]{0,1}\s?(?:([.\d]+)\s?[-\s"_\u02ba\u030b\u030e\u05f4\u201c\u201d\u201e\u201f\u2033\u2036\u3003]{0,1}\s?)?)?)?/, latc: 1, latd: 2, latm: 3, lats: 4, lngc: 5, lngd: 6, lngm: 7, lngs: 8 },
           {name: 'DMS+', regex: /([.\d]+)\s?(?:[-\s_\u00b0\u00baDd\u02da\u030a\u030a\u0325\u2070\u2218\u309a\u309c]{1}\s?(?:([.\d]+)\s?[-\s'_\u05f3\u02b9\u02bc\u02c8\u0301\u05f3\u2018\u2019\u201a\u201b\u2032\u2035\ua78c]{0,1}\s?(?:([.\d]+)\s?[-\s"_\u02ba\u030b\u030e\u05f4\u201c\u201d\u201e\u201f\u2033\u2036\u3003]{0,1}\s?)?)?)?([NSns]{1})[-_\s,]*([.\d]+)(?:[-\s_\u00b0\u00baDd\u02da\u030a\u030a\u0325\u2070\u2218\u309a\u309c]{1}\s?(?:([.\d]+)\s?[-\s'_\u05f3\u02b9\u02bc\u02c8\u0301\u05f3\u2018\u2019\u201a\u201b\u2032\u2035\ua78c]{0,1}\s?(?:([.\d]+)\s?[-\s"_\u02ba\u030b\u030e\u05f4\u201c\u201d\u201e\u201f\u2033\u2036\u3003]{0,1}\s?)?)?)?([WEwe]{1})/, latd: 1, latm: 2, lats: 3, latc: 4, lngd: 5, lngm: 6, lngs: 7, lngc: 8 },
           {name: 'QlandkartGT', regex: /([NSns]{1})([.\d]+)[d_\u00b0\u00ba\u02da\u030a\u0325\u309c\u309a\u2070\u2218]{1}\s?([.\d]+)[-\s'_\u2032\u2035\u02b9]+([WEwe]{1})([.\d]+)[d_\u00b0\u00ba\u02da\u030a\u0325\u309c\u309a\u2070\u2218]{1}\s?([.\d]+)[-\s'_\u2032\u2035\u02b9]*/, latc: 1, latd: 2, latm: 3, lngc: 4, lngd: 5, lngm: 6 },
           {name: 'Proj4', regex: /(\d+)d(\d+)'([.\d]+)"([WE]{1})\s+(\d+)d(\d+)'([.\d]+)"([NS]{1})/, latd: 5, latm: 6, lats: 7, latc: 8, lngd: 1, lngm: 2, lngs: 3, lngc: 4 },
           {name: 'lat/lng text', regex: /m?[Ll](?:at)?[\s=](-?\d+\.?\d*)[&\s,]+m?[Ll](?:on|ng|g)[\s=](-?\d+\.?\d*)/, latd: 1, lngd: 2},
           {name: 'lat/lng', regex: /(-?\d+\.?\d*)\s?[,\s]+(-?\d+\.?\d*)/, latd: 1, lngd: 2},
           {name: 'loose lat/lng', regex: /(-?\d+\.?\d*)\D+?(-?\d+\.?\d*)/, latd: 1, lngd: 2}
         ];
         // $log.debug('text:', text);
         for (var i = 0, n = formats.length; i < n; ++i) {
           found = text.match(formats[i].regex);
           if (found) {
             // $log.debug('Matches with:', formats[i].name);
             // $log.debug('Matches:', found);
             switch (formats[i].name) {
             case 'plus+code':
               // $log.debug('Converting plus+code ', found[1]);
               try {
                 coord = OpenLocationCode.decode(found[1]);
               } catch(ex) {
                 $log.error(ex);
               }
               if (coord) {
                 lat.deg = coord.latitudeCenter;
                 lng.deg = coord.longitudeCenter;
               }
               break;
             case 'osgb36':
               if (bngMap[found[1]]) {
                 // $log.debug('Grid map:', found[1], bngMap[found[1]]);
                 if (bngMap[found[1]].length === 3) {
                   x = Number(found[2].padEnd(5, '0')) + Number(bngMap[found[1]].charAt(1)) * 100000;
                   y = Number(found[3].padEnd(5, '0')) + Number(bngMap[found[1]].charAt(2)) * 100000 + 1000000;
                 } else {
                   x = Number(found[2].padEnd(5, '0')) + Number(bngMap[found[1]].charAt(0)) * 100000;
                   y = Number(found[3].padEnd(5, '0')) + Number(bngMap[found[1]].charAt(1)) * 100000;
                 }
               } else if (found[4] && found[5]) {
                 x = Number(found[4]);
                 y = Number(found[5]);
               }
               // $log.debug('x,y:', x, y);
               try {
                 p4 = proj4('EPSG:27700', 'WGS84', [x, y]);
                 // $log.debug('p4:', p4);
                 lat.deg = p4[1];
                 lng.deg = p4[0];
               } catch(ex) {
                 $log.error(ex);
               }
               break;
             case 'IrishGrid':
               if (irishGridMap[found[1]]) {
                 // $log.debug('Grid map:', found[1], irishGridMap[found[1]]);
                 x = Number(found[2].padEnd(5, '0')) + Number(irishGridMap[found[1]].charAt(0)) * 100000;
                 y = Number(found[3].padEnd(5, '0')) + Number(irishGridMap[found[1]].charAt(1)) * 100000;
               } else if (found[4] && found[5]) {
                 x = Number(found[4]);
                 y = Number(found[5]);
               }
               // $log.debug('x,y:', x, y);
               try {
                 p4 = proj4('EPSG:29902', 'WGS84', [x, y]);
                 // $log.debug('p4:', p4);
                 lat.deg = p4[1];
                 lng.deg = p4[0];
               } catch(ex) {
                 $log.error(ex);
               }
               break;
             case 'ITM':
               x = Number(found[1]);
               y = Number(found[2]);
               // $log.debug('x,y:', x, y);
               try {
                 p4 = proj4('EPSG:2157', 'WGS84', [x, y]);
                 // $log.debug('p4:', p4);
                 lat.deg = p4[1];
                 lng.deg = p4[0];
               } catch(ex) {
                 $log.error(ex);
               }
               break;
             default:
               if (formats[i].latd) lat.deg = Number.parseFloat(found[formats[i].latd]);
               if (formats[i].latm) lat.min = Number.parseFloat(found[formats[i].latm]);
               if (formats[i].lats) lat.sec = Number.parseFloat(found[formats[i].lats]);
               if (formats[i].latc) lat.c = found[formats[i].latc];
               if (formats[i].lngd) lng.deg = Number.parseFloat(found[formats[i].lngd]);
               if (formats[i].lngm) lng.min = Number.parseFloat(found[formats[i].lngm]);
               if (formats[i].lngs) lng.sec = Number.parseFloat(found[formats[i].lngs]);
               if (formats[i].lngc) lng.c = found[formats[i].lngc];
               break;
             }
             break;
           }
         }
         return {lat: lat, lng: lng};
       };

       var convertDmsCoordsToDegreeCoords = function(c) {
         var lat, lng;
         // $log.debug('coord:', c);
         // lat = Math.round((c.lat.deg + (c.lat.min + (c.lat.sec / 60)) / 60) * 100000000) / 100000000;
         lat = Math.round((c.lat.deg + (isFinite(c.lat.min) ? c.lat.min / 60 : 0) + (isFinite(c.lat.sec) ? c.lat.sec / 3600 : 0)) * 100000000) / 100000000;
         lng = Math.round((c.lng.deg + (isFinite(c.lng.min) ? c.lng.min / 60 : 0) + (isFinite(c.lng.sec) ? c.lng.sec / 3600 : 0)) * 100000000) / 100000000;
         if (c.lat < 0 || (c.lat.c && 'Ss'.indexOf(c.lat.c) !== -1)) {
           lat = -lat;
         }
         if (c.lng < 0 || (c.lng.c && 'Ww'.indexOf(c.lng.c) !== -1)) {
           lng = -lng;
         }
         return {lat: lat, lng: lng};
       };

       var parseTextAsDegrees = function(text) {
         return convertDmsCoordsToDegreeCoords(parseGeoLocation(text));
       };

       var convertToFormat = function(lat, lng, format, positionFormat) {
         var retval, p4, x, y, i, grid;
         // $log.debug('Format:', format);
         // clip illegal numeric values
         if (lng < -180) {
           lng = -180;
         } else if (lng > 180) {
           lng = 180;
         }
         if (lat < -90) {
           lat = -90;
         } else if (lat > 90) {
           lat = 90;
         }
         switch(format) {
         case 'plus+code':
           try {
             retval = OpenLocationCode.encode(lat, lng, OpenLocationCode.CODE_PRECISION_EXTRA);
           } catch(ex) {
             $log.error(ex);
           }
           break;
         case 'osgb36':
           retval = '';
           // $log.debug('lat:', lat, 'lng:', lng);
           if (lat && lng) {
             try {
               p4 = proj4('WGS84', 'EPSG:27700', [lng, lat]);
               // $log.debug('proj4:', p4);
               if (p4 && p4[0] && p4[1]) {
                 x = '' + Math.round(p4[0]);
                 y = '' + Math.round(p4[1]);
                 x = x.padStart(6, '0');
                 y = y.padStart(6, '0');
                 if (y >= 1000000) {
                   i = 'N' + x[0] + y[1];
                 } else {
                   i = x[0] + y[0];
                 }
                 // $log.debug('Index:', i);
                 grid = Object.keys(bngMap).find(function(e) {
                   return bngMap[e] === i;
                 });
                 if (grid) {
                   retval = grid + ' ' + x.slice(1) + ' ' + (y >= 1000000 ? y.slice(2) : y.slice(1)) + ' / OSGB36 ';
                 }
               }
               // $log.debug('proj4:', i, x, y);
               retval += x + ', ' + y;
               // $log.debug('retval:', retval);
             } catch(ex) {
               $log.error(ex);
               retval = 'Error';
             }
           }
           break;
         case 'IrishGrid':
           retval = '';
           // $log.debug('lat:', lat, 'lng:', lng);
           if (lat && lng) {
             try {
               p4 = proj4('WGS84', 'EPSG:29902', [lng, lat]);
               // $log.debug('proj4:', p4);
               if (p4 && p4[0] && p4[1]) {
                 x = '' + Math.round(p4[0]);
                 y = '' + Math.round(p4[1]);
                 x = x.padStart(6, '0');
                 y = y.padStart(6, '0');
                 i = x[0] + y[0];
                 // $log.debug('Index:', i);
                 grid = Object.keys(irishGridMap).find(function(e) {
                   return irishGridMap[e] === i;
                 });
                 if (grid) {
                   retval = grid + ' ' + x.slice(1) + ' ' + (y >= 1000000 ? y.slice(2) : y.slice(1)) + ' / IG ';
                 } else {
                   retval = 'IG ';
                 }
               }
               // $log.debug('proj4:', i, x, y);
               retval += x + ', ' + y;
               // $log.debug('retval:', retval);
             } catch(ex) {
               $log.error(ex);
               retval = 'Error';
             }
           }
           break;
         case 'ITM':
           retval = '';
           // $log.debug('lat:', lat, 'lng:', lng);
           if (lat && lng) {
             try {
               p4 = proj4('WGS84', 'EPSG:2157', [lng, lat]);
               // $log.debug('proj4:', p4);
               if (p4 && p4[0] && p4[1]) {
                 x = '' + Math.round(p4[0]);
                 y = '' + Math.round(p4[1]);
                 x = x.padStart(6, '0');
                 y = y.padStart(6, '0');
                 retval = 'ITM ';
               }
               // $log.debug('proj4:', i, x, y);
               retval += x + ', ' + y;
               // $log.debug('retval:', retval);
             } catch(ex) {
               $log.error(ex);
               retval = 'Error';
             }
           }
           break;
         default:
           positionFormat = positionFormat ? positionFormat : 'lat,lng';
           retval = formatPosition(formatCoordinates(lat, format, 'lat'), formatCoordinates(lng, format, 'lng'), positionFormat);
           break;
         }
         return retval;
       };

const parseLocationText = function(text) {
  return parseTextAsDegrees(text);
};
