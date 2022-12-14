<!-- -*- mode: markdown; -*- vim: set tw=78 ts=4 sts=0 sw=4 noet ft=markdown norl: -->

# Trip Server 2

## Introduction and History

TRIP is a web application supporting trip recording and itinerary planning.

The intended use is for a hiker, mountain-biker or other adventurer, to be
able to publish and share their planned itinerary, then subsequently log their
positions at intervals, allowing someone else to be able to monitor their
progress.

In the event of contact being lost, the plans and tracking information can be
passed to rescue services etc., to assist with locating the missing
adventurer.

Trip Server 2 will be a port of [Trip Server][trip-server], written mostly in
C++.  It is far from complete.  It can be run alongside an existing Trip
Server version 1.10.x installation.

The Trip application consists of two primary components, [a server
application][trip-server], written in [ECMAScript][] (a JavaScript standard),
running under [Node.js][] and [a browser application][trip-web-client], also
written in ECMAScript using [AngularJS][], a web framework.

Subsequently, Google's support for AngularJS has ended with a recommendation
of migration to [Angular 2+](https://angular.io/).

Maintaining support for Trip v1 required a not inconsiderable amount of work,
mostly relating to upgrading dependencies, frequently due to security
vulnerabilities in underlying components.  Coupled with supply-chain attacks
within the [npm](https://www.npmjs.com) eco-system, I'm of the view that the
support impact of development in such an architecture is unacceptably high.

As migrating to Angular 2+ is not trivial and with no reassurance that a
similar upgrade will not be necessary in the near future, I was extremely
reluctant to simply follow the upgrade path.

Consequently to these issues, I wanted a solution that had few dependencies,
ideally with those minimal dependencies being on widely used libraries
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
[PostgreSQL database][PostgreSQL], [PostGIS][], XML, JSON and YAML.

The intention during development of version 2 is to maintain database
compatibility as much as possible, with an `--upgrade` option to the Trip
Server version 2 to upgrade a version 1 database to version 2.  Also, the
desire is to keep the YAML configuration file requiring minimal changes.

### Proposed Features

The following features are proposed:

* Remote tracking server???client applications such as
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

## Documentation

This document describes building the application.  Once built and installed,
you can view the application's documentation with the
[info](https://www.gnu.org/software/texinfo/) viewer available in the Debian
`info` package:

	$ info trip-server

The user guide can also be viewed with:

	$ info trip-user-guide

## Source Control

The source is maintained in a Git repository which can be cloned with:

	$ git clone --recursive git://www.fdsd.co.uk/trip-server-2.git

## Building

These instructions are for building and installing from the source
distribution tarball, which contains additional artefacts to those maintained
under Git source control.  Building from a cloned Git repository requires
additional packages to be installed as described below.

### Debian

For Debian version 11 (Bullseye).

Minimal packages required to build from the source distribution tarball:

- g++
- gawk
- libboost-locale-dev
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

[Bootstrap]: https://getbootstrap.com
[OpenLayers]: https://openlayers.org

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

To build from source other than a tarball release, e.g. a git clone, examine
the contents of `./provisioning/bootstrap.sh` to see which packages are
installed using `dnf`.

### FreeBSD

Minimal packages required to build from the source distribution tarball, for
Fedora version 36.

- boost-all
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
- gawk
- gdal (optional)
- intltool
- nlohmann-json
- pkgconfig
- postgresql13-server
- postgis3 +postgresql13
- yaml-cpp

**Note:** If `make distcheck` fails on macOS, install the `texinfo` and
`texlive` packages from [MacPorts][], as the behaviour of the system isntalled
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
