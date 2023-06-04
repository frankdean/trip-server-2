<!-- -*- mode: markdown; -*- vim: set tw=78 ts=4 sts=0 sw=4 noet ft=markdown norl: -->

# Changelog

## 2.0.0-rc.5

- Fix title missing from map attributions
- Implement view raw markdown option for shared itineraries
- Use standard syslog instead of `logger` class
- Replace hard-coded HTTP headers with configured values when fetching tiles
- Include Docker in Vagrant VM
- Disable Boost deprecated header warnings
- Updated documentation
    - Include documentation in Docker build
    - Document how to install user and application documentation
- Link `Help` menu URL to locally installed user guide
- Allow overriding the URL to the user guide in `trip-server.yaml`
- Replace hard-coded maximum tracked locations with configuration option
- Generate Dockerfile during `configure`, setting current package version
