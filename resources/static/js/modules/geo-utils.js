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

export { distance, perpendicularDistance, simplify };

const haversine = function(angle)
{
  return Math.pow(Math.sin(angle / 2.0), 2.0);
};

/// Degrees to radians
const d2r = function(d) {
  return d * Math.PI / 180.0;
};

/// Radians to degrees
const r2d = function(r) {
  return r * 180.0 / Math.PI;
};

const distance = function(lng1, lat1, lng2, lat2) {
  const earth_mean_radius_kms = 6371.0;
  const x1 = d2r(lng1);
  const y1 = d2r(lat1);
  const x2 = d2r(lng2);
  const y2 = d2r(lat2);
  return Math.asin(
    Math.sqrt(
      haversine(y2 - y1)
        + (1.0 - haversine(y1 - y2) - haversine(y1 + y2))
        * haversine(x2 - x1)
    )
  ) * 2.0 * earth_mean_radius_kms;
};

// https://en.wikipedia.org/wiki/Heron's_formula to calculate area
const perpendicularDistance = function(x1, y1, x2, y2, x0, y0) {
  const a = distance(x0, y0, x1, y1);
  const b = distance(x1, y1, x2, y2);
  const c = distance(x2, y2, x0, y0);
  const s = (a + b + c) / 2;
  const area = Math.sqrt(s * (s - a) * (s - b) * (s - c));
  // Calculate height from area
  return area * 2.0 / b;
};

const perpendicularDistancePointLine = function(p, p1, p2) {
  return perpendicularDistance(p1[0], p1[1], p2[0], p2[1], p[0], p[1]);
};

const simplify = function(coords, epsilon) {
  // console.debug('Points before simplify', coords.length, epsilon);
  const retval = RDPppd(coords, epsilon);
  // console.debug('Points after simplify', retval.length, epsilon);
  return retval;
};

/*
*** Ramer Douglas Peucker

The Ramer-Douglas–Peucker algorithm is an algorithm for reducing the number of points in a curve that is approximated by a series of points.
It does so by "thinking" of a line between the first and last point in a set of points that form the curve.
It checks which point in between is farthest away from this line.
If the point (and as follows, all other in-between points) is closer than a given distance 'epsilon', it removes all these in-between points.
If on the other hand this 'outlier point' is farther away from our imaginary line than epsilon, the curve is split in two parts.
The function is recursively called on both resulting curves, and the two reduced forms of the curve are put back together.

1) From the first point up to and including the outlier
2) The outlier and the remaining points.

I hope that by looking at this source code for my Ramer Douglas Peucker implementation you will be able to get a correct reduction of your dataset.

@licence Feel free to use it as you please, a mention of my name is always nice.

Marius Karthaus
http://www.LowVoice.nl

*
*/


// this is the implementation with shortest Distance (as of 2013-09 suggested by the wikipedia page. Thanks Edward Lee for pointing this out)
function RDPsd(points,epsilon){
    var firstPoint=points[0];
    var lastPoint=points[points.length-1];
    if (points.length<3){
        return points;
    }
    var index=-1;
    var dist=0;
    for (var i=1;i<points.length-1;i++){
        var cDist=distanceFromPointToLine(points[i],firstPoint,lastPoint);

        if (cDist>dist){
            dist=cDist;
            index=i;
        }
    }
    if (dist>epsilon){
        // iterate
        var l1=points.slice(0, index+1);
        var l2=points.slice(index);
        var r1=RDPsd(l1,epsilon);
        var r2=RDPsd(l2,epsilon);
        // concat r2 to r1 minus the end/startpoint that will be the same
        var rs=r1.slice(0,r1.length-1).concat(r2);
        return rs;
    }else{
        return [firstPoint,lastPoint];
    }
}


// this is the implementation with perpendicular Distance
function RDPppd(points,epsilon){
    var firstPoint=points[0];
    var lastPoint=points[points.length-1];
    if (points.length<3){
        return points;
    }
    var index=-1;
    var dist=0;
    for (var i=1;i<points.length-1;i++){
        var cDist=findPerpendicularDistance(points[i],firstPoint,lastPoint);
        if (cDist>dist){
            dist=cDist;
            index=i;
        }
    }
    if (dist>epsilon){
        // iterate
        var l1=points.slice(0, index+1);
        var l2=points.slice(index);
        var r1=RDPppd(l1,epsilon);
        var r2=RDPppd(l2,epsilon);
        // concat r2 to r1 minus the end/startpoint that will be the same
        var rs=r1.slice(0,r1.length-1).concat(r2);
        return rs;
    }else{
        return [firstPoint,lastPoint];
    }
}


function findPerpendicularDistance(p, p1,p2) {

    // if start and end point are on the same x the distance is the difference in X.
    var result;
    var slope;
    var intercept;
    if (p1[0]==p2[0]){
        result=Math.abs(p[0]-p1[0]);
    }else{
        slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
        intercept = p1[1] - (slope * p1[0]);
        result = Math.abs(slope * p[0] - p[1] + intercept) / Math.sqrt(Math.pow(slope, 2) + 1);
    }

    return result;
}


// code as suggested by Edward Lee

var distanceFromPointToLine = function (p,a,b){
    // convert array to object to please Edwards code;
    p={x:p[0],y:p[1]};
    a={x:a[0],y:a[1]};
    b={x:b[0],y:b[1]};
    return Math.sqrt(distanceFromPointToLineSquared(p,a,b));
}

//This is the difficult part. Commenting as we go.
var distanceFromPointToLineSquared = function (p, i, j){
	var lineLength = pointDistance(i,j);//First, we need the length of the line segment.
	if(lineLength==0){	//if it's 0, the line is actually just a point.
		return pointDistance(p,a);
	}
	var t = ((p.x-i.x)*(j.x-i.x)+(p.y-i.y)*(j.y-i.y))/lineLength;

	//t is very important. t is a number that essentially compares the individual coordinates
	//distances between the point and each point on the line.

	if(t<0){	//if t is less than 0, the point is behind i, and closest to i.
		return pointDistance(p,i);
	}	//if greater than 1, it's closest to j.
	if(t>1){
		return pointDistance(p,j);
	}
	return pointDistance(p, { x: i.x+t*(j.x-i.x),y: i.y+t*(j.y-i.y)});
	//this figure represents the point on the line that p is closest to.
}

//returns distance between two points. Easy geometry.
var pointDistance = function (i,j){
	return sqr(i.x-j.x)+sqr(i.y-j.y);
}

//just to make the code a bit cleaner.
const sqr = function (x){
	return x*x;
}
