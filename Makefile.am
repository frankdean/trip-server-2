SUBDIRS = src po tests doc

# When running `distcheck` build with additional features that are normally
# disabled by default
AM_DISTCHECK_CONFIGURE_FLAGS = \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir) \
	--enable-cairo --enable-directory-listing --enable-tui

# if HAVE_SYSTEMD
#     systemdsystemunit_DATA = \
#     conf/trip.service
# endif

EXTRA_DIST = m4/ChangeLog  .gitignore stamp-h1 \
	RELEASE_PROCEDURE.md \
	Vagrantfile \
	docker-build.sh \
	conf/trip.service \
	doc/trip-server-ref.sgml \
	docker-compose-map-demo.yml \
	docker-compose-map-import.yml \
	docker-compose-map-server.yml \
	provisioning/apache2/etc-apache2-conf-available-renderd.conf \
	provisioning/apache2/etc-apache2-conf-available-trip.conf \
	provisioning/bootconfig.sh \
	provisioning/bootstrap.sh \
	provisioning/debian-on-macos-setup.sh \
	provisioning/qemu-init.sh \
	provisioning/podman-setup.sh \
	provisioning/docker/tile_server_run.sh \
	provisioning/init.sh \
	provisioning/logrotate.d/trip \
	provisioning/nginx/LICENSE \
	provisioning/nginx/conf.d/trip.conf \
	provisioning/nginx/index.html \
	provisioning/nginx/sites-available/trip \
	provisioning/nginx/trip.conf \
	provisioning/patches/libpqxx6-tools-splitconfig.patch \
	provisioning/schema/10_init-trip-db.sh \
	provisioning/schema/10_trip_role.sql \
	provisioning/schema/11_init-gis-db.sh \
	provisioning/schema/20_schema.sql \
	provisioning/schema/30_permissions.sql \
	provisioning/schema/35_user_roles.sql \
	provisioning/schema/40_path_colors.sql \
	provisioning/schema/50_georef_formats.sql \
	provisioning/schema/60_waypoint_symbols.sql \
	provisioning/schema/90_test-data.sql \
	provisioning/schema/create_gis_indexes.sh \
	provisioning/schema/revoke_all.sql \
	provisioning/systemd/trip-server-2.service \
	trip-server.yaml

ACLOCAL_AMFLAGS = -I m4

dist-hook:
	rm -f `find $(distdir)/resources -type f '(' -name '.*' -o -name '*~' ')'`

dist_sysconf_DATA = conf/trip-server-dist.yaml
dist_doc_DATA = README.md CHANGELOG.md \
	conf/trip.service

# nobase_dist_pkgdata_DATA = resources/static/css/trip.css \
# 	resources/static/js/map.js

cssdir = $(pkgdatadir)/resources/static/css
dist_css_DATA = resources/static/css/trip.css
icondir = $(pkgdatadir)/resources/static/icons
dist_icon_DATA = resources/static/icons/LICENSE \
	resources/static/icons/map-delete-btn.svg \
	resources/static/icons/map-edit-btn.svg \
	resources/static/icons/map-exit-btn.svg \
	resources/static/icons/map-layers-btn.svg \
	resources/static/icons/map-live-btn.svg \
	resources/static/icons/map-route-btn.svg \
	resources/static/icons/map-waypoint-btn.svg
jsdir = $(pkgdatadir)/resources/static/js
dist_js_DATA = resources/static/js/change-password.js \
	resources/static/js/itinerary-map.js \
	resources/static/js/itinerary-path-join.js \
	resources/static/js/itinerary-route-edit.js \
	resources/static/js/itinerary-segment-edit.js \
	resources/static/js/itinerary-track-edit.js \
	resources/static/js/itinerary-waypoint.js \
	resources/static/js/simplify-path.js \
	resources/static/js/tracking-map.js
jsmoddir = $(pkgdatadir)/resources/static/js/modules
dist_jsmod_DATA = resources/static/js/modules/geo-utils.js \
	resources/static/js/modules/itinerary.js \
	resources/static/js/modules/location-service.js \
	resources/static/js/modules/map.js \
	resources/static/js/modules/utils-service.js
bsver = bootstrap-$(BOOTSTRAP_VERSION)-dist
nobase_dist_pkgdata_DATA = resources/static/$(bsver)/LICENSE \
	resources/static/favicon.ico

bscssdir = $(pkgdatadir)/resources/static/bootstrap-$(BOOTSTRAP_VERSION)-dist/css
dist_bscss_DATA = \
	resources/static/bootstrap-$(BOOTSTRAP_VERSION)-dist/css/bootstrap.min.css \
	resources/static/bootstrap-$(BOOTSTRAP_VERSION)-dist/css/bootstrap.min.css.map

bsjsdir = $(pkgdatadir)/resources/static/bootstrap-$(BOOTSTRAP_VERSION)-dist/js
dist_bsjs_DATA = resources/static/bootstrap-$(BOOTSTRAP_VERSION)-dist/js/bootstrap.bundle.min.js \
	resources/static/bootstrap-$(BOOTSTRAP_VERSION)-dist/js/bootstrap.bundle.min.js.map

oldir = $(pkgdatadir)/resources/static/openlayers-$(OPENLAYERS_VERSION)
dist_ol_DATA = \
	resources/static/openlayers-$(OPENLAYERS_VERSION)/LICENSE.md \
	resources/static/openlayers-$(OPENLAYERS_VERSION)/ol.css \
	resources/static/openlayers-$(OPENLAYERS_VERSION)/ol.js.map \
	resources/static/openlayers-$(OPENLAYERS_VERSION)/ol.js

proj4jsdir = $(pkgdatadir)/resources/static/proj4js-$(PROJ4JS_VERSION)
dist_proj4js_DATA = resources/static/proj4js-$(PROJ4JS_VERSION)/LICENSE.md
proj4js2dir = $(pkgdatadir)/resources/static/proj4js-$(PROJ4JS_VERSION)/dist
dist_proj4js2_DATA = \
	resources/static/proj4js-$(PROJ4JS_VERSION)/dist/proj4-src.js \
	resources/static/proj4js-$(PROJ4JS_VERSION)/dist/proj4.js

olcdir = $(pkgdatadir)/resources/static/open-location-code-$(OLC_VERSION)
dist_olc_DATA = \
	resources/static/open-location-code-$(OLC_VERSION)/openlocationcode.js \
	resources/static/open-location-code-$(OLC_VERSION)/openlocationcode.min.js \
	resources/static/open-location-code-$(OLC_VERSION)/LICENSE

zxcvbndir = $(pkgdatadir)/resources/static/zxcvbn-$(ZXCVBN_VERSION)
dist_zxcvbn_DATA = resources/static/zxcvbn-$(ZXCVBN_VERSION)/LICENSE.txt
zxcvbn2dir = $(pkgdatadir)/resources/static/zxcvbn-$(ZXCVBN_VERSION)/dist
dist_zxcvbn2_DATA = \
	resources/static/zxcvbn-$(ZXCVBN_VERSION)/dist/zxcvbn.js \
	resources/static/zxcvbn-$(ZXCVBN_VERSION)/dist/zxcvbn.js.map

# The leading minus sign tells make to ignore any errors from the shell

install-data-hook:
	-test ! -L $(pkgdatadir)/resources/static/doc && \
	$(LN_S) $(docdir) $(pkgdatadir)/resources/static/doc

uninstall-local:
	-rm -f $(pkgdatadir)/resources/static/doc

uninstall-hook:
	-rmdir -p $(bscssdir)
	-rmdir -p $(bsjsdir)
	-rmdir -p $(oldir)
	-rmdir -p $(cssdir)
	-rmdir -p $(icondir)
	-rmdir -p $(jsdir)
	-rmdir -p $(jsmoddir)
	-rmdir -p $(proj4jsdir)
	-rmdir -p $(proj4js2dir)
	-rmdir -p $(olcdir)
	-rmdir -p $(zxcvbndir)
	-rmdir -p $(zxcvbn2dir)
	-rmdir $(docdir)
	-rmdir $(pkgdatadir)
