<!-- -*- mode: markdown; -*- vim: set tw=78 ts=4 sts=0 sw=4 noet ft=markdown norl: -->

# Trip Server 2

## Introduction and History

TRIP is a web application supporting trip recording and itinerary planning.

The intended use is for a hiker, mountain-biker or other adventurer, to be
able to publish and share their planned itinerary, then subsequently log their
positions at intervals, allowing someone else to monitor their progress.

In the event of contact being lost, the plans and tracking information can be
passed to rescue services etc., to assist with locating the missing
adventurer.

Trip Server 2 is a port of [Trip Server v1][trip-server], written mostly in
C++.  Compared to the previous version, it is not fully complete, but now
supports all the primary tracking, itinerary management and sharing functions,
sufficient to support its primary purpose. It can be run alongside an existing
Trip Server version 1.11.x installation.

The original Trip v1 application consists of two primary components, [a server
application][trip-server], written in [ECMAScript][] (a JavaScript standard),
running under [Node.js][] and [a browser application][trip-web-client], also
written in ECMAScript using [AngularJS][], a web framework.

Subsequently, Google's support for AngularJS has ended with a recommendation
of migration to [Angular 2+](https://angular.io/).

Maintaining support for Trip v1 required a not inconsiderable amount of work,
mostly relating to upgrading dependencies, frequently due to security
vulnerabilities in underlying components.  Coupled with supply-chain attacks
within the [npm](https://www.npmjs.com) eco-system, I'm of the view that the
ongoing support impact of development in such an architecture is unacceptably
high.

As migrating to Angular 2+ is not trivial and with no reassurance that a
similar upgrade will not be necessary in the future, I was extremely
reluctant to simply follow the upgrade path.

Consequently, I want a solution that has as few dependencies as practical,
ideally with those minimal dependencies being on widely used libraries which are
unlikely to force much re-work of any application code.  I've used many
languages and experimented with some of the more popular modern languages, but
I don't see any meeting my desire for something that will remain largely
backward compatible, well supported for years to come and with low maintenance
overheads.

Reviewing the version history of a GUI application I previously wrote in C++,
I've only had one occasion where I *needed* to make a change in over a decade,
caused by the deprecation of [GConf](https://en.wikipedia.org/wiki/GConf)
(used for holding configuration settings).  It was a trivial change, and an
improvement to replace it with a [YAML](https://yaml.org) configuration file.

C++ scores highly on backwards compatibility, is governed by a good standards
committee process (ISO) and has steadily evolved into a powerful and widely
supported development language.  Using modern C++ practices *can* produce
stable, reliable, easy to maintain code.  With plenty of mature, stable
libraries for the more significant things we need; primarily support for
[PostgreSQL database][PostgreSQL], [PostGIS][], XML, JSON and YAML.  So far,
I am very satisfied with the results.

The intention during development of version 2 is to maintain database
compatibility as much as possible, with an `--upgrade` option to the Trip
Server version 2 to upgrade a version 1 database to version 2.  Also, the
desire is to keep the YAML configuration file requiring minimal changes.

### Proposed Features

The following features have been implemented:

* Remote tracking server—client applications such as
  [TripLogger Remote for iOS][TripLogger] &ndash;
  ([on the App Store](https://apps.apple.com/us/app/triplogger-remote/id1322577876?mt=8))
  or
  [GPSLogger for Android][GPSLogger] can be used to submit locations to the
  server.

* Sharing tracks with others.

* Viewing tracks on a map provided by a tile server, e.g. OpenStreetMap tiles.

* Creating and sharing itineraries using the Markdown markup language.

* Using the map, interactively creating routes and waypoints for an itinerary.

* Uploading and downloading routes, tracks and waypoints for an itinerary as a
  [GPX][] file.

* Viewing routes, tracks and waypoints of an itinerary on the map.

* Splitting and joining routes and tracks.

* Deleting individual points from routes and tracks.

* Account maintenance &ndash; uploading [TripLogger][] settings.

* Account maintenance &ndash; password change.

* Itinerary shares report &ndash; list shared itineraries including nicknames.

* Itinerary search by coordinate and radius.

* Administration &ndash; user management.

* Administration &ndash; tile usage report.

## Documentation

This document describes building the application.  Once built and installed,
you can view the application's documentation with the
[info](https://www.gnu.org/software/texinfo/) viewer available in the Debian
`info` package:

	$ info trip-server

The user guide can also be viewed with:

	$ info trip-user-guide

The [user guide](https://www.fdsd.co.uk/trip-server-2/latest/docs/user-guide/) and
[application
manual](https://www.fdsd.co.uk/trip-server-2/latest/docs/application-guide/)
are available online.

## Source Control

The source is maintained in a Git repository which can be cloned with:

	$ git clone --recursive git://www.fdsd.co.uk/trip-server-2.git

## Demo Options

### Play With Docker

[play]: https://labs.play-with-docker.com "Play with Docker"

The application can be run as two or three containers in the
[Play-with-docker][play] environment.

1.  Create a network for the containers to share:

		$ docker network create trip-server

1.  Optionally, run the
	[Docker container for OpenStreetMap tile server](https://github.com/Overv/openstreetmap-tile-server)

	**Note:** I couldn't get this container to complete the import in the
	Play-with-docker environment, possibly due to insufficient resources.  It
	sometimes randomly failed and otherwise failed importing
	`water-polygons-split` which uses a lot of resources.  Nonetheless, the
	process is documented here as it may be useful in other environments.

	1.  The import process temporarily requires a lot of memory.  We need to
		create and mount a swap file:

			$ sudo dd if=/dev/zero of=/swap bs=1G count=2
			$ sudo chmod 0600 /swap
			$ sudo mkswap /swap
			$ sudo swapon /swap

	1.  Create Docker volumes for storing the database and caching map tiles:

			$ docker volume create osm-data
			$ docker volume create osm-tiles

	1.  Import OSM data for Monaco:

		It is recommended to use a small region to minimise resource usage.

			$ docker run -v osm-data:/data/database/ \
			-e DOWNLOAD_PBF=https://download.geofabrik.de/europe/monaco-latest.osm.pbf \
			-e DOWNLOAD_POLY=https://download.geofabrik.de/europe/monaco.poly \
			overv/openstreetmap-tile-server:2.3.0 import

		The process must complete without error.  Check the final output
        carefully.  If it fails, it is necessary to delete and re-create the
		Docker `osm-data` volume.

			$ docker volume rm osm-data
			$ docker volume create osm-data

	1.  Once the import completes successfully, run the map tile server:

			$ docker run -v osm-tiles:/data/tiles/ -v osm-data:/data/database/ \
			--network trip-server --network-alias map \
			-d overv/openstreetmap-tile-server:2.3.0 run

1.  Run the Trip Server database container:

			$ docker run --network trip-server --network-alias postgis \
			-e POSTGRES_PASSWORD=secret -d fdean/trip-database:1.11.4

1.  Run the Trip Server web container:

			$ docker run --network trip-server -e TRIP_SIGNING_KEY=secret \
			-e TRIP_RESOURCE_SIGNING_KEY=secret -e POSTGRES_PASSWORD=secret \
			CONFIGURE_TILE_SERVER=no \
			--publish 8080:8080 -d fdean/trip-server-2

		Set `CONFIGURE_TILE_SERVER` to `yes` if you have the map tile
		container running.  When not set to `yes`, dummy map tiles are created
		showing their x, y and z values.

## Building

These instructions are for building and installing from the source
distribution tarball, which contains additional artefacts to those maintained
under Git source control.  Building from a cloned Git repository requires
additional packages to be installed as described below.

Generally the application is built and installed with:

	$ ./configure
	$ make
	$ sudo make install

More detailed build instructions are below.

See the `PostgreSQL Database Configuration` section in the instructions for
[TripServer v1][trip-server] to install PostgreSQL and create the database.
Upgrade the database to support Trip Server v2 by running:

	$ trip-server --upgrade

### Additional Resources

The following resources are included in the [tarball distributions of this
application](http://www.fdsd.co.uk/trip-server-2/) and will need to be
separately installed under `./resources/static/` when building from a git
clone of the repository.  See `./Makefile.am` for details of their expected
locations.

- [Bootstrap][]
- [open-location-code][]
- [proj4js][]
- [zxcvbn][]

[Bootstrap]: https://getbootstrap.com "Powerful, extensible, and feature-packed frontend toolkit"
[open-location-code]: https://github.com/google/open-location-code "a library to generate short codes, called "plus codes", that can be used as digital addresses where street addresses don't exist"
[OpenLayers]: https://openlayers.org "OpenLayers makes it easy to put a dynamic map in any web page"
[proj4js]: https://github.com/proj4js/proj4js "JavaScript library to transform coordinates from one coordinate system to another, including datum transformations"
[zxcvbn]: https://github.com/dropbox/zxcvbn "Low-Budget Password Strength Estimation"

### Debian

For Debian version 11 (Bullseye).

Minimal packages required to build from the source distribution tarball:

- g++
- gawk
- libboost-locale-dev
- libcairomm-1.0-dev (optional)
- libcmark-dev
- libgdal-dev (optional)
- libpqxx-dev
- libpugixml-dev
- libyaml-cpp-dev
- make
- nlohmann-json3-dev
- uuid-dev

To build the application:

	$ ./configure
	$ make

You may need to add arguments to the `./configure` command.  Run `./configure
--help` to see available options.  E.g. on a Raspberry Pi running Debian 10
(Buster) or Debian 11 (Bullseye), the following is required to successfully
link against the Boost Locale library:

	$ ./configure --with-boost-locale=boost_locale

See <http://www.randspringer.de/boost/ucl-sbs.html> for help with the Boost
library arguments.

GDAL is only required for extracting elevation data from elevation tile
datasets.  If you don't need this feature, disable GDAL.

	$ ./configure --disable-gdal

Cairo is only used to create dummy map tiles which may be useful in a
development environment where you do not wish to use a map tile server.
Enable it with:

	$ ./configure --enable-cairo

The version of [nlohmann-json] in Debian 10 (Buster) is too old for this
application.  Uninstall the package and download version `v3.11.2` or later of
`json.hpp` from <https://github.com/nlohmann/json/releases> and manually
install it under `/usr/local/include`

	$ sudo mkdir /usr/local/include/nlohmann
	$ sudo cp ~/Downloads/json.hpp /usr/local/include/nlohmann/

Building on Debian 10 (Buster) also produces warnings about an ABI change in
GCC 7.1.
See

- <https://stackoverflow.com/questions/48149323/what-does-the-gcc-warning-project-parameter-passing-for-x-changed-in-gcc-7-1-m>
- <https://stackoverflow.com/questions/52020305/what-exactly-does-gccs-wpsabi-option-do-what-are-the-implications-of-supressi>

Optionally, run the tests:

	$ make check

Install:

	$ sudo make install

The build requires resources from the [Bootstrap][] and [OpenLayers][]
distributions.  These are included in the
[distribution tarballs of this
application](http://www.fdsd.co.uk/trip-server-2/)
or can be downloaded from the respective websites.  (In this case, view the
contents of `./Makefile.am` to determine the required versions and directory
structure for the build.)

Additional packages required to build from a Git clone:

- autoconf
- autoconf-archive
- automake
- autopoint
- intltool

To re-create the required Gnu autotools files:

	$ autoreconf -i
	$ automake --add-missing --copy

Optionally install the `uuid-runtime` package which runs a daemon that
`libuuid` uses to create secure UUIDs.

To run Trip Server as a daemon, create a system user, e.g.

	$ sudo adduser trip --system --group --home /nonexistent --no-create-home

For further ideas on configuring your environment, see the scripts and files
under the `./provisioning` directory, which can be used to create a
development environment using [Vagrant][].

[Vagrant]: https://www.vagrantup.com "Development Environments Made Easy"

### Fedora

Minimal packages required to build from the source distribution tarball, for
Fedora version 36.

The application requires a version 6.x of [libpqxx][] installed, which is
older than that in this version of Fedora.  See the 'libpqxx' section below
for instructions on installing `libpqxx`.

The application also requires the `nlohmann/json` package, which is not
included in the Fedora distribution.  Follow the instructions in the
'nlohmann/json' section below to install it.  Tested on Fedora with
`nlohmann/json` version 3.11.2.

- gcc
- gawk
- boost-devel
- cairomm-devel (optional)
- gdal-devel (optional)
- libpqxx-devel
- libpq-devel
- yaml-cpp-devel
- pugixml-devel
- libuuid-devel

Optionally install the `uuidd` package which runs a daemon that `libuuid` uses
to create secure UUIDs.

GDAL is only required for extracting elevation data from elevation tile
datasets.  If you don't need this feature, disable GDAL.

	$ ./configure --disable-gdal

Cairo is only used to create dummy map tiles which may be useful in a
development environment where you do not wish to use a map tile server.
Enable it with:

	$ ./configure --enable-cairo

To build from source other than a tarball release, e.g. a git clone, examine
the contents of `./provisioning/bootstrap.sh` to see which packages are
installed using `dnf`.

### FreeBSD

Minimal packages required to build from the source distribution tarball, for
Fedora version 36.

- boost-all
- cmark
- gdal (optional)
- yaml-cpp
- postgresql13-client
- pugixml
- e2fsprogs-libuuid
- nlohmann-json

The application requires a version 6.x of [libpqxx][] installed, which is
older than that in this version of FreeBSD.  See the 'libpqxx' section below
for instructions on installing `libpqxx`.

GDAL is only required for extracting elevation data from elevation tile
datasets.  If you don't need this feature, disable GDAL.

	$ ./configure --disable-gdal

Cairo is only used to create dummy map tiles which may be useful in a
development environment where you do not wish to use a map tile server.
Enable it with:

	$ ./configure --enable-cairo

To build from source other than a tarball release, e.g. a git clone, examine
the contents of `./provisioning/bootstrap.sh` to see which packages are
installed using `pkg`.

### macOS

The application requires a version 6.x of [libpqxx][] installed, which is
older than that in [MacPorts][].  See the 'libpqxx' section below for
instructions on installing `libpqxx`.  Tested on macOS with `libpqxx` version
6.4.8.

Add `CXXFLAGS='-g -O0'` to disable compiler optimisation.

To build from a Git clone, install the following ports from [MacPorts][]:

- autoconf
- automake
- autoconf-archive
- boost
- cairomm (optinal)
- cmark
- gawk
- gdal (optional)
- intltool
- nlohmann-json
- pkgconfig
- postgresql13-server
- postgis3 +postgresql13
- yaml-cpp

**Note:** If `make distcheck` fails on macOS, install the `texinfo` and
`texlive` packages from [MacPorts][], as the behaviour of the system installed
`/usr/bin/texi2dvi` differs from the GNU version.

### Dependencies

This section describes how to manually download and installed required
dependencies, should they not be available as a package.

#### GDAL

GDAL is only required for extracting elevation data from elevation tile
datasets.  If you don't need this feature, disable GDAL.

	$ ./configure --disable-gdal

#### libpqxx

Download, build and install the latest 6.x release of libpqxx from
<https://github.com/jtv/libpqxx/releases/tag/6.4.8>.

`libpqxx` needs the `doxygen` and `xmlto` packages installed to build the
refence documentation and tutorial.  Pass `--disable-documentation` to the
`./configure` command if you wish to skip building the documentation.

When running the `./configure` command to build this application, define the
`PKG_CONFIG_PATH` to include where `libpqxx.pc` and d`libpq.pc` are installed,
e.g.:

	./configure PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$(pg_config --libdir)/pkgconfig"

#### nlohmann/json

Download the appropriate version of `json.hpp` from
<https://github.com/nlohmann/json/releases>.

	$ cd /usr/local/include
	$ sudo mkdir nlohmann
	$ sudo cp ~/Downloads/json.hpp /usr/local/include/nlohmann

[AngularJS]: https://angularjs.org
[Apache]: http://httpd.apache.org/ "an open-source HTTP server for modern operating systems including UNIX and Windows"
[Debian]: https://www.debian.org "a free operating system (OS) for your computer"
[ECMAScript]: https://en.wikipedia.org/wiki/ECMAScript
[GPSLogger]: http://code.mendhak.com/gpslogger/ "A battery efficient GPS logging application"
[Git]: http://git-scm.com/ "a free and open source distributed version control system designed to handle everything from small to very large projects with speed and efficiency"
[libpqxx]: http://pqxx.org/ "The official C++ client API for PostgreSQL"
[Linux]: https://www.kernel.org/
[MacPorts]: http://www.macports.org/ "MacPorts Home Page"
[Markdown]: http://daringfireball.net/projects/markdown/ "A text-to-HTML conversion tool for web writers"
[Nginx]: https://nginx.org/ "HTTP and reverse proxy server, a mail proxy server, and a generic TCP/UDP proxy server"
[Node.js]: https://nodejs.org/ "A JavaScript runtime built on Chrome's V8 JavaScript engine"
[OpenStreetMap]: http://www.openstreetmap.org/ "OpenStreetMap"
[PostGIS]: https://postgis.net "Spatial and Geographic objects for PostgreSQL"
[PostGIS]: https://postgis.net "Spatial and Geographic objects for PostgreSQL"
[PostgreSQL]: https://www.postgresql.org "A powerful, open source object-relational database system"
[PostgreSQL]: https://www.postgresql.org "A powerful, open source object-relational database system"
[RaspberryPi]: https://www.raspberrypi.org
[Traccar Client]: https://www.traccar.org/client/
[TripLogger]: https://www.fdsd.co.uk/triplogger/ "TripLogger Remote for iOS"
[Vagrant]: https://www.vagrantup.com "Development Environments Made Easy"
[VirtualBox]: https://www.virtualbox.org "A x86 and AMD64/Intel64 virtualization product"
[docker]: https://www.docker.com "Securely build and share any application, anywhere"
[gpx]: http://www.topografix.com/gpx.asp "The GPX Exchange Format"
[semver]: http://semver.org
[trip-server]: https://www.fdsd.co.uk/trip-server/ "TRIP - Trip Recording and Itinerary Planner"
[trip-web-client]: https://www.fdsd.co.uk/trip-web-client-docs/
