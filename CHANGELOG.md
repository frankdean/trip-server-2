<!-- -*- mode: markdown; -*- vim: set tw=78 ts=4 sts=0 sw=4 noet ft=markdown norl: -->

# Changelog

## 2.1.1-rc.2

- Test database is ready on startup with retries

	This is needed where systems (e.g. `podman-compose`) start the web
	container before database container is ready, failing to respect the
	`depends_on` configuration option

## 2.1.1-rc.1

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
