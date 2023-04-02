SUBDIRS = src po tests doc

EXTRA_DIST = m4/ChangeLog  .gitignore \
	Vagrantfile \
	provisioning/bootconfig.sh \
	provisioning/bootstrap.sh \
	provisioning/init.sh \
	provisioning/logrotate.d/trip \
	provisioning/nginx/LICENSE \
	provisioning/nginx/conf.d/trip.conf \
	provisioning/nginx/index.html \
	provisioning/nginx/sites-available/trip \
	provisioning/nginx/trip.conf \
	provisioning/systemd/trip-server-2.service \
	resources

ACLOCAL_AMFLAGS = -I m4

dist-hook:
	rm -f `find $(distdir)/resources -type f '(' -name '.*' -o -name '*~' ')'`

dist_sysconf_DATA = conf/trip-server-dist.yaml
dist_doc_DATA = README.md

# nobase_dist_pkgdata_DATA = resources/static/css/trip.css \
# 	resources/static/js/map.js

cssdir = $(pkgdatadir)/resources/static/css
dist_css_DATA = resources/static/css/trip.css
jsdir = $(pkgdatadir)/resources/static/js
dist_js_DATA = resources/static/js/tracking-map.js \
	resources/static/js/itinerary-map.js \
	resources/static/js/itinerary-segment-edit.js \
	resources/static/js/itinerary-track-edit.js \
	resources/static/js/itinerary-waypoint.js \
	resources/static/js/simplify-path.js
jsmoddir = $(pkgdatadir)/resources/static/js/modules
dist_jsmod_DATA = resources/static/js/modules/itinerary.js \
	resources/static/js/modules/location-service.js \
	resources/static/js/modules/map.js \
	resources/static/js/modules/utils-service.js
bsver = bootstrap-$(BOOTSTRAP_VERSION)-dist
nobase_dist_pkgdata_DATA = resources/static/$(bsver)/LICENSE

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
	resources/static/openlayers-$(OPENLAYERS_VERSION)/ol.js.LICENSE.txt \
	resources/static/openlayers-$(OPENLAYERS_VERSION)/ol.js.map \
	resources/static/openlayers-$(OPENLAYERS_VERSION)/ol.js

proj4jsdir = $(pkgdatadir)/resources/static/proj4js-$(PROJ4JS_VERSION)
dist_proj4js_DATA = \
	resources/static/proj4js-$(PROJ4JS_VERSION)/proj4-src.js \
	resources/static/proj4js-$(PROJ4JS_VERSION)/proj4.js \
	resources/static/proj4js-$(PROJ4JS_VERSION)/LICENSE.md

olcdir = $(pkgdatadir)/resources/static/open-location-code-$(OLC_VERSION)
dist_olc_DATA = \
	resources/static/open-location-code-$(OLC_VERSION)/openlocationcode.js \
	resources/static/open-location-code-$(OLC_VERSION)/openlocationcode.min.js \
	resources/static/open-location-code-$(OLC_VERSION)/LICENSE

uninstall-hook:
	-rmdir -p $(bscssdir)
	-rmdir -p $(bsjsdir)
	-rmdir -p $(oldir)
	-rmdir -p $(cssdir)
	-rmdir -p $(jsdir)
	-rmdir -p $(jsmoddir)
	-rmdir -p $(proj4jsdir)
	-rmdir -p $(olcdir)
