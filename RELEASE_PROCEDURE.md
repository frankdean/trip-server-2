# Trip Version 2 - Release Procedure

## Changelog

1.  Update `./CHANGELOG.md` with the changes in this release.

## Version Control

1.  Update `master` branch in `trip-server-common` sub-project

1.  Push `master` branch in `trip-server-common` sub-project

1.  Checkin the updated `master` branch from the `trip-server-common` sub-project

## Build

1.  Update version number in `configure.ac`

1.  Create the distribution tarballs

		$ autoreconf -i

	on Debian:

		$ ./configure --enable-cairo --enable-tui --enable-maintainer-mode
		$ make -j 4 check
		$ MAKEFLAGS='-j 4' make distcheck

	on macOS:

		$ ./configure CXX=/opt/local/bin/clang++-mp-20 \
		'CXXFLAGS=-Wno-deprecated-builtins -Wno-deprecated-literal-operator '\
		'-Wno-unused-but-set-variable' \
		PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:"\
		"/opt/local/lib/proj9/lib/pkgconfig:"\
		"$(pg_config --libdir)/pkgconfig" \
		--enable-cairo --enable-tui --enable-maintainer-mode
		$ make -j 8 check
		$ PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:"\
		"/opt/local/lib/proj9/lib/pkgconfig:"\
		"/opt/local/lib/postgresql15/pkgconfig" \
		DISTCHECK_CONFIGURE_FLAGS='CXX=/opt/local/bin/clang++-mp-20 '\
		'CXXFLAGS=-Wno-deprecated-builtins\ -Wno-deprecated-literal-operator\ '\
		'-Wno-unused-but-set-variable' \
		MAKEFLAGS='-j 8' make distcheck

1.  Create SHA256 sums for the tarballs

1.  Sign the tarballs:

		gpg --sign --detach-sign --armor $TARBALL

1.  Copy the tarballs to the download site

1.  Build PDF and HTML docs

		$ make html pdf

1.  Check in the changed files, including the updated `po` language files

## Docker

1.  Optionally, check the `Dockerfile` has been updated to the use the correct
	release version number.  The version information is updated by `configure`
	when `Dockerfile` is created from `Dockerfile.in`.

		$ grep 'ARG TRIP_SERVER_VERSION' Dockerfile
		$ grep LABEL Dockerfile Dockerfile-postgis

1.	Test the Dockerfile

		$ docker build --platform=linux/arm64 --build-arg MAKEFLAGS='-j 4' \
		-t fdean/trip-server-2:latest .
		$ docker compose up --no-recreate --detach
		$ docker logs --follow trip-server-2_web_1

	Stop the container, optionally with (use the `--volumes` switch to also
    remove the database volume):

		$ docker compose down --volumes

1.  Build the Docker images, e.g.

	1.  Optionally, Update `Dockerfile-postgis` to use the latest
		[PostgreSQL build](https://hub.docker.com/_/postgres).

	1.  Build the `trip-database` and `trip-server` images:

			$ DOCKER=podman PUSH=n ./docker-build.sh

## Installation

	$ ./configure
	$ make
	$ make html pdf
	$ sudo make install install-html install-pdf

## Release

1.  Copy the builds, SHA256 sums, HTML and PDF docs to the Trip Server 2
    website

1.  Update the index page with details of the new release

1.  Update the symbolic link to point to the latest docs

1.  Tag the `master` branch of the parent project with release version number

1.  Push the master branch and check
    <https://www.fdsd.co.uk/trip-server-2/readme.html> has been updated

1.  Push Docker images:

		$ MAKEFLAGS='-j 4' DOCKER=podman ./docker-build.sh

## Validation

1.  Check the website links for all the user documentation

1.  Check the `Help` menu item correctly links to the latest user guide

1.  Check the tarballs download

1.  Download the checksums file and validate against the downloaded tarballs
