<!-- -*- mode: markdown; -*- vim: set tw=78 ts=4 sts=0 sw=4 noet ft=markdown norl: -->

# Changelog

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
