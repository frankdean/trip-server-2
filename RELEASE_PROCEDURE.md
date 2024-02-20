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

		$ ./configure
		$ make check
		$ make distcheck

1.  Create SHA256 sums for the tarballs

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

1.  Build the Docker images.  Either build them manually as follows, or add
    the `--build` option to the `compose up` command in the section below.

	1.  Optionally, build the database image.  This only needs updating if
		there have been any database schema changes.

		Update `Dockerfile-postgis` to use the latest
		[PostgreSQL build](https://hub.docker.com/_/postgres).

			$ cd ./trip-server-2
			$ docker pull docker.io/library/postgres:15-bookworm
			$ docker build -f Dockerfile-postgis -t fdean/trip-database:latest .

	1.  Build the `trip-server-2` image:

		*  Run the Docker build:

				$ cd ./trip-server-2
				$ docker pull docker.io/library/debian:bookworm-slim
				$ docker build -t fdean/trip-server-2:latest .

			The `--no-cache` option may be required if Docker uses a cached
			version of the `COPY` command when copying the distribution tarball to
			the image.

		*  Optionally, check the image labels with:

				$ docker image inspect --format='{{println .Config.Labels}}' \
				  fdean/trip-server-2:latest
				$ docker image inspect --format='{{println .Config.Labels}}' \
				  fdean/trip-database:latest

1.	Test the Dockerfile

	Omit the `--build` option if the images were built in the earlier steps.

		$ docker compose up -d --build
		$ docker compose logs --follow

	Stop the container with (use the `--volumes` switch to also remove
    the database volume):

		$ docker compose down --volumes

## Installation

	$ ./configure
	$ make
	$ make html pdf
	$ sudo make install install-html install-pdf

## Release

1.  Tag the `master` branch of the parent project with release version number

1.  Copy the builds, SHA256 sums, HTML and PDF docs to the Trip Server 2
    website

1.  Update the index page with details of the new release

1.  Update the symbolic link to point to the latest docs

1.  Tag the master branch with the release number

1.  Push the master branch and check
    <https://www.fdsd.co.uk/trip-server-2/readme.html> has been updated

1.  Push Docker images:

		$ docker tag fdean/trip-server-2:latest fdean/trip-server-2:$VERSION
		$ docker push fdean/trip-server-2:latest
		$ docker push fdean/trip-server-2:$VERSION

1.  Optionally, push database image:

		$ docker tag fdean/trip-database:latest fdean/trip-database:$VERSION
		$ docker push fdean/trip-database:latest
		$ docker push fdean/trip-database:$VERSION

## Validation

1.  Check the website links for all the user documentation

1.  Check the `Help` menu item correctly links to the latest user guide

1.  Check the tarballs download

1.  Download the checksums file and validate against the downloaded tarballs
