# Trip Version 2 - Release Procedure

## Changelog

T.B.D.

## Version Control

1.  Update `master` branch in `trip-server-common` sub-project

1.  Checkin the updated `master` branch from the `trip-server-common` sub-project

1.  Tag the `master` branch of the parent project with release version number

## Build

1.  Create the distribution tarballs

		$ ./configure
		$ make check
		$ make distcheck

1.  Create SHA256 sums for the tarballs

1.  Copy the tarballs to the download site

1.  Build PDF and HTML docs

		$ make html pdf

## Release

1.  Copy the builds, SHA256 sums, HTML and PDF docs to the Trip Server 2
    website

1.  Update the index page with details of the new release

1.  Update the symbolic link to point to the latest docs

1.  Tag the master branch with the release number

1.  Push the master branch and check
    <https://www.fdsd.co.uk/trip-server-2/readme.html> has been updated

## Validation

1.  Check the website links for all the user documentation

1.  Check the tarballs download

1.  Download the checksums file and validate against the downloaded tarballs
