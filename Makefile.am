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
dist_js_DATA = resources/static/js/map.js

bsver = bootstrap-5.2.1-dist
nobase_dist_pkgdata_DATA = resources/static/$(bsver)/LICENSE

bscssdir = $(pkgdatadir)/resources/static/bootstrap-5.2.1-dist/css
dist_bscss_DATA = \
	resources/static/bootstrap-5.2.1-dist/css/bootstrap.min.css \
	resources/static/bootstrap-5.2.1-dist/css/bootstrap.min.css.map

bsjsdir = $(pkgdatadir)/resources/static/bootstrap-5.2.1-dist/js
dist_bsjs_DATA = resources/static/bootstrap-5.2.1-dist/js/bootstrap.min.js \
	resources/static/bootstrap-5.2.1-dist/js/bootstrap.min.js.map

oldir = $(pkgdatadir)/resources/static/openlayers-7.0.0-legacy
dist_ol_DATA = \
	resources/static/openlayers-7.0.0-legacy/LICENSE.md \
	resources/static/openlayers-7.0.0-legacy/ol.css \
	resources/static/openlayers-7.0.0-legacy/ol.js.LICENSE.txt \
	resources/static/openlayers-7.0.0-legacy/ol.css.map \
	resources/static/openlayers-7.0.0-legacy/ol.js.map \
	resources/static/openlayers-7.0.0-legacy/ol.js
