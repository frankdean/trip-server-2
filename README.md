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
C++. It can be run alongside an existing Trip Server version 1.11.x
installation, as version 2 maintains database compatibility with version 1,
with an `--upgrade` option to upgrade a version 1 database to version 2.

### Features

The following features have been implemented:

* Remote tracking server—client applications such as
  [TripLogger Remote for iOS][TripLogger] &ndash;
  ([on the App Store](https://apps.apple.com/us/app/triplogger-remote/id1322577876?mt=8))
  or
  [GPSLogger for Android][GPSLogger] can be used to submit locations to the
  server.

* Sharing tracks with others.

* Viewing tracks on a map provided by a tile server, e.g. [OpenStreetMap][] tiles.

* Creating and sharing itineraries using the [Markdown][] markup language.

* Using the map, interactively creating routes and waypoints for an itinerary.

* Uploading and downloading routes, tracks and waypoints for an itinerary as a
  [GPX][] file.

* Viewing routes, tracks and waypoints of an itinerary on the map.

* Splitting, simplifying and joining routes and tracks.

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

The [user guide][trip user guide] and [application manual][trip application manual]
are available online.

[trip user guide]: https://www.fdsd.co.uk/trip-server-2/latest/docs/user-guide/
[trip application manual]: https://www.fdsd.co.uk/trip-server-2/latest/docs/application-guide/

## Source Control

The source is maintained in a [Git][] repository which can be cloned with:

	$ git clone --recursive git://www.fdsd.co.uk/trip-server-2.git
	$ cd ./trip-server-2/trip-server-common
	$ git submodule update --init --recursive

## Demo Options

### Docker

You can use [Docker][] to run the application, using `docker compose`.

- `docker-compose.yml` runs the application without a tile server.
- `docker-compose-map-demo.yml` runs the application with a tile server
  containing map data for Luxembourg.

1.  Navigate to a directory containing one of those docker compose files.

2.  If you're running the tile server option, create the following docker
    volumes:

        $ docker volume create osm-data
		$ docker volume create osm-tiles

3.  Start the containers with:

        $ docker compose -f docker-compose-map-demo.yml up -d

    On the first occasion the tile server is started, it'll take a little
    while to create the initial database.  It will also probably be quiet slow
    to both startup and also render the tiles.  Use the `docker container
    logs` command to view the progress.

4.  Use a browser to navigate to `http://<docker-host>:8080/trip/app` to view
    the application.  Login with the credentials listed below in the 'Play
    with Docker' section.  Refer to the [user guide][trip user guide] and
    [application manual][trip application manual] for more information.

5.  Stop the containers with:

        $ docker compose -f docker-compose-map-demo.yml down

### Play with Docker

[play]: https://labs.play-with-docker.com "Play with Docker"

1.  Use [Play with Docker][play] to run the application with some test data in
    a browser, without having to install anything.

1.  Navigate to [Play with Docker][play] and login using a Docker ID.  If you
    do not have one, you will see the option to sign up after clicking `Login`
    then `Docker`.

1.  Click `+ ADD NEW INSTANCE`

1.  Create a network for the containers to share:

		$ docker network create trip-server

1.  Optionally, run the
	[Docker container for OpenStreetMap tile server](https://github.com/Overv/openstreetmap-tile-server)

	**Note:** I couldn't get this container to complete the import in the
	Play-with-docker environment, possibly due to insufficient resources.  It
	sometimes randomly failed and otherwise failed importing
	`water-polygons-split` which uses a lot of resources.  Nonetheless, the
	process is documented here as it may be useful in other environments.
	Also, 'Play with Docker' doesn't seem to have sufficient resources to run
	the `fdean/tile-server` image.

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
			-e CONFIGURE_TILE_SERVER=no \
			--publish 8080:8080 -d fdean/trip-server-2

	Set `CONFIGURE_TILE_SERVER` to `yes` if you have the map tile
	container running.  When not set to `yes`, dummy map tiles are created
	showing their x, y and z values.

1.  Once the application is running a link titled `8080` will be shown next to
    the `OPEN PORT` button at the top of the page.  Click on the `8080` link
    to open a new browser window to the running web server.  If the port
    number doesn't show up, click on `OPEN PORT` and enter the port number as
    `8080`.

1.  Login using one of the following users and credentials:

	- user@trip.test  rasHuthlutcew7
	- admin@trip.test 7TwilfOrucFeug

1.  Select `Help` from the menu to view the user guide.

## Building

These instructions are for building and installing from the source
distribution tarball, which contains additional artefacts to those maintained
under Git source control.  Building from a cloned Git repository requires
additional packages to be installed as described below.

Generally the application is built and installed with:

	$ ./configure
	$ make
	$ sudo make install

Add `CXXFLAGS='-g -O0'` to disable compiler optimisation.  E.g.

	$ ./configure 'cxxflags=-g -o0'

Optionally install the HTML and PDF documentation:

	$ make html pdf
	$ sudo make install-pdf install-html

By default links to the user guide within the application (e.g. the `Help`
menu option), serve the locally installed user guide.  The location can be
overridden in the application's configuration file, see the `Configuration`
section in the `trip-server` info manual.

See the `PostgreSQL Database Configuration` section in the instructions for
[TripServer v1][trip-server] to install [PostgreSQL][] and create the database.
Upgrade the database to support Trip Server v2 by running:

	$ trip-server --upgrade

The upgrade option is re-runable.  If the `pgcrypto` extension has already
been created, a warning is issued but can be ignored.

More detailed instructions for building on different platforms are in
the following sections.

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
[open-location-code]: https://github.com/google/open-location-code "a library to generate short codes, called “plus codes”, that can be used as digital addresses where street addresses don't exist"
[OpenLayers]: https://openlayers.org "OpenLayers makes it easy to put a dynamic map in any web page"
[proj4js]: https://github.com/proj4js/proj4js "JavaScript library to transform coordinates from one coordinate system to another, including datum transformations"
[zxcvbn]: https://github.com/dropbox/zxcvbn "Low-Budget Password Strength Estimation"

### Optional Text-based User Interface (TUI)

The application can optionally be built with the `--enable-tui` configure
option to include an interactive TUI which can be used to create an initial
admin user.

1.  Download and install the [Final Cut][finalcut] library.  **Note** when
    building Final Cut for [Linux][], to enable mouse support you need to also
    install the `gpm` library.  If you need to install additional libraries to
    fix a failed build, re-run Final Cut's `configure` script so that it
    re-configures the build to use the libraries.

	On Debian 11 (Bullseye), install the following packages:

	- libgpm-dev
	- libncurses-dev

2.  Include the `--enable-tui` option when running `configure`, e.g.:

		$ ./configure --enable-tui
		$ make

3.  After creating the initial database, (see the `PostgreSQL Setup` section
    of the application manual (`info trip-server`)), execute `trip-server`
    with the `--interactive` option:

		$ ./src/trip-server --interactive

4.  Select the option to create a new user from the menu.

[finalcut]: https://github.com/gansm/finalcut/tree/main "A text-based widget toolkit"

### Debian

For Debian version 11 (Bullseye).

Minimal packages required to build from the source distribution tarball:

- g++
- gawk
- libboost-dev
- libboost-locale-dev
- libcairomm-1.0-dev (optional)
- libcmark-dev
- libgdal-dev (optional)
- libpqxx-dev
- libpugixml-dev
- libyaml-cpp-dev
- make
- nlohmann-json3-dev
- postgis (optional)
- postgresql (optional)
- texinfo (optional for building HTML documentation)
- texlive (optional for building PDF documentation)
- uuid-dev

To build the application:

	$ ./configure
	$ make

You may need to add arguments to the `./configure` command.  Run `./configure
--help` to see available options.  E.g. on a Raspberry Pi running Debian 10
(Buster) or Debian 11 (Bullseye), the following is required to successfully
link against the Boost Locale library:

	$ ./configure --with-boost-locale=boost_locale

If you are developing the application and need to add flags to the
`make distcheck` command, set them in the `DISTCHECK_CONFIGURE_FLAGS`
variable, e.g.

	$ DISTCHECK_CONFIGURE_FLAGS='--with-boost-locale=boost_locale' make distcheck

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

Building with GCC on ARM devices (e.g. Raspberry Pi) produces warnings about
an ABI change in GCC 7.1.

Optionally, disable the warning by passing `-Wno-psabi` in `CXXFLAGS`, e.g.:

	$ ./configure 'CXXFLAGS=-Wno-psabi'

- <https://gcc.gnu.org/gcc-7/changes.html>
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

	$ aclocal
	$ autoheader
	$ autoreconf -i
	$ automake --add-missing --copy

Optionally install the `uuid-runtime` package which runs a daemon that
`libuuid` uses to create secure UUIDs.

To run Trip Server as a daemon, create a system user, e.g.

	$ sudo adduser trip --system --group --home /nonexistent --no-create-home

For further ideas on configuring your environment, see the scripts and files
under the `./provisioning` directory, which can be used to create a
development environment using [Vagrant][]. See the `Testing and Developing
Trip` section of the application manual (`info trip-server`) for more
information on using and running the application with Vagrant and [Qemu][].

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

[fedora_download]: https://fedoraproject.org/server/download/

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

[FreeBSD_download]: https://www.freebsd.org/where/

### macOS

The application requires a version 6.x of [libpqxx][] installed, which is
older than that in [MacPorts][].  See the 'libpqxx' section below for
instructions on installing `libpqxx`.  Tested on macOS with `libpqxx` version
6.4.8.

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
`PKG_CONFIG_PATH` to include where `libpqxx.pc` and d`libpq.pc` are installed.
Also, if your default compiler is not `clang`, you should specify the `clang`
compiler by defining the `CXX` environment variable when building both
`libpqxx` and Trip.

e.g.:

	./configure \
	PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$(pg_config --libdir)/pkgconfig" \
	CXX=/usr/bin/g++

#### nlohmann/json

Download the appropriate version of `json.hpp` from
<https://github.com/nlohmann/json/releases>.

	$ cd /usr/local/include
	$ sudo mkdir nlohmann
	$ sudo cp ~/Downloads/json.hpp /usr/local/include/nlohmann

## Changelog

See [CHANGELOG](./CHANGELOG.md)

[GPSLogger]: http://code.mendhak.com/gpslogger/ "A battery efficient GPS logging application"
[Git]: http://git-scm.com/ "a free and open source distributed version control system designed to handle everything from small to very large projects with speed and efficiency"
[libpqxx]: http://pqxx.org/ "The official C++ client API for PostgreSQL"
[Linux]: https://www.kernel.org/
[MacPorts]: http://www.macports.org/ "MacPorts Home Page"
[Markdown]: http://daringfireball.net/projects/markdown/ "A text-to-HTML conversion tool for web writers"
[OpenStreetMap]: http://www.openstreetmap.org/ "OpenStreetMap"
[PostgreSQL]: https://www.postgresql.org "A powerful, open source object-relational database system"
[Qemu]: https://www.qemu.org "A generic and open source machine emulator and virtualizer"
[TripLogger]: https://www.fdsd.co.uk/triplogger/ "TripLogger Remote for iOS"
[Vagrant]: https://www.vagrantup.com "Development Environments Made Easy"
[Docker]: https://www.docker.com
[gpx]: http://www.topografix.com/gpx.asp "The GPX Exchange Format"
[trip-server]: https://www.fdsd.co.uk/trip-server/ "TRIP - Trip Recording and Itinerary Planner"
