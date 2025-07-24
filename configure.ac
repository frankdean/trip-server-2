#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([trip],[2.5.4-rc.4],[frank.dean@fdsd.co.uk])
TRIP_SOURCE_URL=[https://www.fdsd.co.uk/trip-server-2/]
AC_SUBST([TRIP_SOURCE_URL])
AC_DEFINE_UNQUOTED([TRIP_SOURCE_URL], ["${TRIP_SOURCE_URL}"], [The download URL for this application's source code])
BOOTSTRAP_VERSION='5.3.7'
OPENLAYERS_VERSION='9.2.4'
# WARNING: proj4js releases include alpha releases without it being stated in
# the Github release version number.  Check the version attribute in
# package.json.
PROJ4JS_VERSION='2.16.1'
OLC_VERSION='1.0.4'
ZXCVBN_VERSION='4.4.2'
AC_COPYRIGHT([Copyright (C) 2022-2025 Frank Dean])
dnl May fix an issue where the make files perform a rebuild of configure.ac,
dnl Makefile.am etc.  Run ./configure with --enable-maintainer-mode to
dnl automatically re-run autoconf scripts.
AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-zip subdir-objects info-in-builddir])
AC_CONFIG_SRCDIR([src/trip_request_factory.cpp])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_TESTDIR([tests])
AC_CONFIG_MACRO_DIRS([m4])

dnl provide options to enable static files, directory listings and keep alive
TRIP_WEB_OPTION_STATIC_FILES([yes])
TRIP_WEB_OPTION_DIRECTORY_LISTING
TRIP_WEB_OPTION_KEEP_ALIVE

AC_ARG_ENABLE([boost-geometry],
    AS_HELP_STRING([--disable-boost-geometry],
	[Disable building using Boost Geometry headers]))
AC_ARG_ENABLE([gdal],
    AS_HELP_STRING([--disable-gdal],
	[Disable building using GDAL library]))
AC_ARG_ENABLE([cairo],
    AS_HELP_STRING([--enable-cairo],
	[Enable building using CAIRO library]))
AC_ARG_ENABLE([tui],
    AS_HELP_STRING([--enable-tui],
	[Enable building Text-based User Interface]))

# Checks for programs.
AC_LANG(C++)
dnl Must set the compiler to use before AX_CXX_COMPILE_STDCXX_17
#AC_PROG_CXX([clang++ g++ c++ gpp aCC CC cxx cc++ cl.exe FCC KCC RCC xlC_r xlC])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX(17)
AC_PROG_INSTALL
AC_PROG_LN_S
AC_CHECK_PROGS([DOCBOOK2TEXI],[docbook2texi docbook2x-texi])
AC_CHECK_PROGS([DOCBOOK2MAN],[docbook2man docbook2x-man])

# Set $host variable
AC_CANONICAL_HOST

# Checks for libraries.
AC_SEARCH_LIBS([pthread_create], [pthread])
AX_BOOST_BASE([1.67],[],[AC_MSG_ERROR([Please install at least version 1.67 of the Boost Locale Library])])
AX_BOOST_LOCALE

PKG_CHECK_MODULES([LIBPQXX], [libpqxx >= 7.7.5 libpqxx < 8 libpq >= 11.14],
			     [AC_SUBST([have_libpqxx7], [1])],
			     [AC_SUBST([have_libpqxx7], [0])]
			     )
AS_IF([test "${have_libpqxx7}" -eq 1],
   [AC_DEFINE_UNQUOTED([HAVE_LIBPQXX7], [${have_libpqxx7}], [Build using libpqxx v7 instead of v6])],
   [PKG_CHECK_MODULES([LIBPQXX], [libpqxx >= 6.2.5 libpqxx < 7 libpq >= 11.14])
])
PKG_CHECK_MODULES([LIBYAML], [yaml-cpp >= 0.6.2])
PKG_CHECK_MODULES([LIBPUGIXML], [pugixml >= 1.11])
PKG_CHECK_MODULES([LIBCMARK], [libcmark >= 0.29])
AS_IF([test "x${enable_cairo}" = xyes], [
   PKG_CHECK_MODULES([LIBCAIROMM], [cairomm-1.0 >= 1.12.2 cairomm-png-1.0 >= 1.12.2])
   AC_DEFINE([HAVE_CAIRO], [1], [Build to use CAIRO library])
])
AM_CONDITIONAL([USE_CAIRO], [test x$enable_cairo = xyes])
AS_IF([test "x${enable_gdal}" != xno], [
   PKG_CHECK_MODULES([LIBGDAL], [gdal >= 3.2.2])
   AC_DEFINE([HAVE_GDAL], [1], [Build to use GDAL library])
   PKG_CHECK_MODULES([LIBPROJ], [proj >= 9.0.0],
      [AC_DEFINE([HAVE_PROJ], [1], [Build to use PROJ library])],
      [AC_MSG_RESULT([${LIBPROJ_PKG_ERRORS} - will not set PROJ search path for proj.db])])
   dnl GDAL_SYSTEM='elevation_tile.$(OBJEXT)'
   dnl AC_SUBST([GDAL_SYSTEM])
])
AM_CONDITIONAL([USE_TUI], [test x$enable_tui = xyes])
AS_IF([test "x${enable_tui}" = xyes], [
   AC_DEFINE([HAVE_TUI], [1], [Build with Text-based User Interface])
   PKG_CHECK_MODULES([FINALCUT], [finalcut >= 0.8.0])
])
AM_CONDITIONAL([USE_GDAL], [test x$enable_gdal != xno])

dnl Check we have a UUID library and header
TRIP_CHECK_UUID_LIBS

CXXFLAGS="${CXXFLAGS} -Wno-deprecated-declarations ${LIBPQXX_CFLAGS} ${LIBYAML_CFLAGS} ${LIBUUID_CFLAGS} ${BOOST_CPPFLAGS} ${LIBPUGIXML_CFLAGS} ${LIBGDAL_CFLAGS} ${LIBPROJ_CFLAGS} ${LIBCAIROMM_CFLAGS} ${LIBCMARK_CFLAGS} ${FINALCUT_CFLAGS}"
dnl CPPFLAGS="${CPPFLAGS} ${XXXXXX_CPPFLAGS}"
LIBS="${LIBS} ${LIBPQXX_LIBS} ${LIBYAML_LIBS} ${LIBUUID_LIBS} ${BOOST_LDFLAGS} ${BOOST_LOCALE_LIB} ${LIBPUGIXML_LIBS} ${LIBGDAL_LIBS} ${LIBPROJ_LIBS} ${LIBCAIROMM_LIBS} ${LIBCMARK_LIBS} ${FINALCUT_LIBS}"

TRIP_CHECK_PQXX_BINARY_CAST

dnl echo "Host_os: ${host_os}"
dnl echo "Host: ${host}"
dnl
# Checks for header files.
AC_HEADER_ASSERT
dnl The following headers check is just to ensure the headers have been
dnl installed in addition to the actual library
AC_CHECK_HEADERS([pqxx/config-public-compiler.h],,
    AC_MSG_ERROR([[Cannot find PostgreSQL C++ client header files, pqxx/config-public-compiler.h]]))
dnl The presence of TargetConditionals.h indicates we're probably building
dnl using Xcode SDK so we can compile differently for macOS and iOS targets.
AC_CHECK_HEADERS([thread getopt.h TargetConditionals.h])
AC_CHECK_HEADERS([nlohmann/json.hpp],,
    [AC_MSG_ERROR([nlohmann/json.hpp not found.  Please install the nlohmann-json package.])],)
dnl Check if the version of libuuid supports the uuid_generate_time_safe method
TRIP_CHECK_UUID_FUNC

AS_IF([test "x${enable_boost_geometry}" != xno], [
   AC_CHECK_HEADERS([boost/geometry.hpp boost/range.hpp boost/geometry/algorithms/simplify.hpp],,
	 [AC_MSG_ERROR([Boost library headers appear to be missing. Check config.log.])])
   AC_DEFINE([HAVE_BOOST_GEOMETRY], [1], [Build to use Boost Geometry headers])
   AC_DEFINE([BOOST_ALLOW_DEPRECATED_HEADERS], [1], [Suppress deprecated header warnings in Boost])
])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADER([string],,
    [AC_MSG_ERROR([Standard C++ library not found])])
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MKTIME
AC_CHECK_FUNCS([strerror])

dnl i18n
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.21])

AM_ICONV

AC_SUBST([BOOTSTRAP_VERSION])
AC_DEFINE_UNQUOTED([BOOTSTRAP_VERSION], ["${BOOTSTRAP_VERSION}"], [The version of Bootstrap])
AC_SUBST([OPENLAYERS_VERSION])
AC_DEFINE_UNQUOTED([OPENLAYERS_VERSION], ["${OPENLAYERS_VERSION}"], [The version of OpenLayers])
AC_SUBST([PROJ4JS_VERSION])
AC_DEFINE_UNQUOTED([PROJ4JS_VERSION], ["${PROJ4JS_VERSION}"], [The version of Proj4js])
AC_SUBST([OLC_VERSION])
AC_DEFINE_UNQUOTED([OLC_VERSION], ["${OLC_VERSION}"], [The version of Open Location Code])
AC_SUBST([ZXCVBN_VERSION])
AC_DEFINE_UNQUOTED([ZXCVBN_VERSION], ["${ZXCVBN_VERSION}"], [The version of zxcvbn])

AC_DEFINE([BUILD_FOR_IOS], [0], "Build for an iOS target")

# Configure and install systemd service file
SYSTEMD_CONF

AC_CONFIG_FILES([Makefile po/Makefile.in
	         doc/Makefile
                 src/Makefile
                 tests/Makefile
		 docker-build.sh
		 Dockerfile
		 Dockerfile-postgis
		 Dockerfile-tile-server
		 Dockerfile-trixie
		 docker-entrypoint.sh
		 docker-compose.yml
                 ],
		 [chmod +x docker-build.sh]
		 )
AC_OUTPUT
