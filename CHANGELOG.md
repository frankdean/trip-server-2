<!-- -*- mode: markdown; -*- vim: set tw=78 ts=4 sts=0 sw=4 noet ft=markdown norl: -->

# Changelog

## 2.3.1

- Fix exception when setting new `date from` tracking query parameter after
  session timeout.
- Log to syslog when an exception is caught when handling a request.

## 2.3.0

- Refresh database connection pool after a broken connection exception

- Updated the upload and download of GPX files to include the OSMAnd XML
  namespace (only if required) and updated the format of the OSMAnd waypoint
  `color` attribute.  The `color` and additional OSMAnd extended waypoint
  attributes are now stored as JSON in a new `external_attributes` column in
  `itinerary_waypoint`.  The **database schema must be updated** using the
  `trip-server --upgrade` option, which also converts and removes the existing
  `color` column.  Make a backup before running the upgrade.

  Consequently the `allowInvalidXsd` configuration option is now redundant.

## 2.2.2

- Update versions of Bootstrap and Proj4js
- Bug fix - invalid SQL

	Version 13 of PostgreSQL is tolerant of a missing space after a parameter,
	but not PostgreSQL 15.

## 2.2.1

- Bug Fix - use session defaults for location search when setting `date from`

## 2.2.0

- Bug fix - password change suggestions are now hidden when no longer relevant
- Show password cracking time with more sensible units
- Clicking on a track point time in the location search sets the `date from`
  field parameter to the selected point's time
- Render path arrows relative to the path's total distance
- When editing paths, zoom to cover the area of the current list of points
- Test database is ready on startup with retries.  This is needed where
  systems (e.g. `podman-compose`) start the web container before the database
  container is ready, failing to respect the `depends_on` configuration
  option.
- Upgrade OpenLayers to 7.5.1
- Upgrade Bootstrap to 5.3.1

## 2.1.0

Improved slider control for simplifying paths making it appear more linear.

- Session timeout can be modified in the configuration file
- Modified HTML access keys to be more consistent
- Separated the configuration file implementation from the shared code-base
- Minor updates to Vagrant configuration and documentation
- Created a Docker configuration to use pre-imported map tile database for
  quick demo

## 2.0.0

- Initial Release
