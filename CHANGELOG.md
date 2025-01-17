<!-- -*- mode: markdown; -*- vim: set tw=78 ts=4 sts=0 sw=4 noet ft=markdown norl: -->

# Changelog

## 2.5.3

- Support loading elevation tiles directly from zip files.

## 2.5.2

- Increase the precision of latitude and longitude values when editing waypoints

## 2.5.1

- Updated to use [OpenLayers][] 9.2.4

- Fix for OpenLayers 9.x - Paths need to have consistent coordinate array
  lengths, either all with elevation data or all without.

## 2.5.0

Refactored to run with [libpqxx][] version 7.x in addition to 6.x.  The
`configure` script first looks for an installed version 7 before falling back
to version 6.

- Allow more general use of Vagrant provisioning scripts in other
  environments, e.g. [Qemu][].  See the `bootstrap.sh` header for
  brief instructions.
- Update Vagrant scripts to use PostgreSQL 15 where available
- Updated to use [OpenLayers][] 9.1.0, [Bootstrap][] 5.3.3 and [proj4js][]
  2.11.0.
- Fix fetching inactive shared tracks.
- Only fetch active user shares when fetching nicknames for itinerary map.
- Fix forbidden resources to be handled as forbidden instead of not authorized.
- Fix read-only viewing of itinerary waypoint.
- Handle invalid position/location when editing waypoints.
- Fix previously input values lost from itinerary waypoint when invalid
  position submitted.
- Fix missing validation rules for latitude and longitude values of itinerary
  waypoint.

## 2.4.2

- Added a favicon
- Include the name of the itinerary in the itinerary page title
- Updated Vagrant virtual machines
- Update proj4js to version 2.9.2
- Update nlohmann-json to v3.11.3 when using Vagrant with Fedora & Rocky Linux
- Update trip-server documentation re importing map tiles
- Fix error in texinfo file
- Fixed formatting errors when generating PDF documentation

## 2.4.1

- Bug fix - invalid SQL

	Version 13 of PostgreSQL is tolerant of a missing space after a parameter,
	but not PostgreSQL 15.

- Bug fix - blank route and track elevations not being filled on file import

- Bug fix - locations not shown on map where no shared locations

	If no other user is sharing locations with the current user, the map fails
	to show any locations for the current user.

- Bug fix - could not view routes or tracks on read-only shared itinerary.

- Bug fix - invalid HTML with read-only view of an itinerary waypoint.

- Removed launching Trip Server v1 from Vagrant scripts.

- Documentation updates.

- Refactored code to avoid global reference to elevation_service.

- Use std::unique_ptr for elevation loading thread instead of standard pointer.

- Improved running with tile server under Docker

- Support including elevation data in Docker container

## 2.4.0

- Implemented a Text-based User Interface to create users.

- Do not listen on socket when upgrading the database or running
  interactively.

- Updated database schema.

	  Added `extended_attributes` column to `itinerary_waypoint` table.  This
	  should have been included in the 2.3.0 release.

	  The change is also achieved by running `trip-server` with the
	  `--upgrade` option.

- Added missing foreign key constraints to the `user_role` table.  The
  **database schema should be updated** using the `trip-server --upgrade`
  option.  The upgrade will fail with a foreign key constraint violation if
  any Admin users have been deleted in the past.  Use the following SQL to
  identify those invalid user ID's:

		SELECT ur.user_id FROM user_role ur
		LEFT JOIN usertable u ON u.id=ur.user_id
		WHERE u.id IS NULL ORDER BY ur.user_id;

	The invalid entries can be deleted with the following SQL:

		DELETE FROM user_role WHERE user_id IN
		(SELECT ur.user_id FROM user_role ur
		LEFT JOIN usertable u ON u.id=ur.user_id
		WHERE u.id IS NULL);

- Fix exception thrown when no map tile usage metrics are available.

- Fix linker warning.

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

[Bootstrap]: https://getbootstrap.com "Powerful, extensible, and feature-packed frontend toolkit"
[OpenLayers]: https://openlayers.org "OpenLayers makes it easy to put a dynamic map in any web page"
[Qemu]: https://www.qemu.org "A generic and open source machine emulator and virtualizer"
[libpqxx]: https://pqxx.org/libpqxx/ "The official C++ client API for PostgreSQL"
[proj4js]: https://github.com/proj4js/proj4js "JavaScript library to transform coordinates from one coordinate system to another, including datum transformations"
