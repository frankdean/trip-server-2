#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([trip],[2.5.3],[frank.dean@fdsd.co.uk])
BOOTSTRAP_VERSION='5.3.3'
OPENLAYERS_VERSION='9.2.4'
PROJ4JS_VERSION='2.11.0'
OLC_VERSION='1.0.4'
ZXCVBN_VERSION='4.4.2'
AC_COPYRIGHT([Copyright (C) 2022-2024 Frank Dean])
dnl May fix an issue where the make files perform a rebuild of configure.ac,
dnl Makefile.am etc.  Run ./configure with --enable-maintainer-mode to
dnl automatically re-run autoconf scripts.
AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-zip subdir-objects info-in-builddir])
AC_CONFIG_SRCDIR([src/trip_request_factory.cpp])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_TESTDIR([tests])

AC_ARG_ENABLE([keep-alive],
    AS_HELP_STRING([--enable-keep-alive],
	[Build to enable HTTP keep-alive (default no)]),
	[use_keep_alive=${enableval}],
	[use_keep_alive=no])
AC_ARG_ENABLE([static-files],
    AS_HELP_STRING([--disable-static-files],
	[Build to allow HTTP serving of static files (default yes)]),
	[use_static_files=${enableval}],
	[use_static_files=yes])
AC_ARG_ENABLE([directory-listing],
    AS_HELP_STRING([--enable-directory-listing],
	[Build to allow HTTP directory listings (default no)]),
	[use_directory_listing=${enableval}],
	[use_directory_listing=no])
AC_ARG_ENABLE([boost-geometry],
    AS_HELP_STRING([--disable-boost-geometry],
	[Build using Boost Geometry headers (default yes)]),
	[use_boost_geometry=${enableval}],
	[use_boost_geometry=yes])
AC_ARG_ENABLE([gdal],
    AS_HELP_STRING([--disable-gdal],
	[Build using GDAL library (default yes)]),
	[use_gdal=${enableval}],
	[use_gdal=yes])
AC_ARG_ENABLE([cairo],
    AS_HELP_STRING([--enable-cairo],
	[Build using CAIRO library (default no)]),
	[use_cairo=${enableval}],
	[use_cairo=no])
AC_ARG_ENABLE([tui],
    AS_HELP_STRING([--enable-tui],
	[Build Text-based User Interface (default no)]),
	[use_tui=${enableval}],
	[use_tui=no])

# Checks for programs.
AC_LANG(C++)
dnl Must set the compiler to use before AX_CXX_COMPILE_STDCXX_17
#AC_PROG_CXX([clang++ g++ c++ gpp aCC CC cxx cc++ cl.exe FCC KCC RCC xlC_r xlC])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_17
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
if test "${have_libpqxx7}" -eq 1; then
   AC_DEFINE_UNQUOTED([HAVE_LIBPQXX7], [${have_libpqxx7}], [Build using libpqxx v7 instead of v6])
else
   PKG_CHECK_MODULES([LIBPQXX], [libpqxx >= 6.2.5 libpqxx < 7 libpq >= 11.14])
fi

PKG_CHECK_MODULES([LIBYAML], [yaml-cpp >= 0.6.2])
PKG_CHECK_MODULES([LIBPUGIXML], [pugixml >= 1.11])
PKG_CHECK_MODULES([LIBCMARK], [libcmark >= 0.29])
if test "x${use_cairo}" = xyes; then
   PKG_CHECK_MODULES([LIBCAIROMM], [cairomm-1.0 >= 1.12.2 cairomm-png-1.0 >= 1.12.2])
   AC_DEFINE([HAVE_CAIRO], [1], [Build to use CAIRO library])
fi
AM_CONDITIONAL([USE_CAIRO], [test x$use_cairo = xyes])
if test "x${use_gdal}" = xyes; then
   PKG_CHECK_MODULES([LIBGDAL], [gdal >= 3.2.2])
   AC_DEFINE([HAVE_GDAL], [1], [Build to use GDAL library])
   PKG_CHECK_MODULES([LIBPROJ], [proj >= 9.0.0],
      [AC_DEFINE([HAVE_PROJ], [1], [Build to use PROJ library])],
      [AC_MSG_RESULT([${LIBPROJ_PKG_ERRORS} - will not set PROJ search path for proj.db])])
   dnl GDAL_SYSTEM='elevation_tile.$(OBJEXT)'
   dnl AC_SUBST([GDAL_SYSTEM])
fi
AM_CONDITIONAL([USE_TUI], [test x$use_tui = xyes])
if test "x${use_tui}" = xyes; then
   AC_DEFINE([HAVE_TUI], [1], [Build with Text-based User Interface])
   PKG_CHECK_MODULES([FINALCUT], [finalcut >= 0.8.0])
fi
AM_CONDITIONAL([USE_GDAL], [test x$use_gdal = xyes])

case $host in
*-*-freebsd* | *-*-linux-*)
    $PKG_CONFIG --exists uuid
    if [[ $? -eq 0 ]]; then
       have_libuuid=yes
       LIBUUID_CFLAGS=$($PKG_CONFIG --cflags uuid)
       LIBUUID_LIBS=$($PKG_CONFIG --libs uuid)
       dnl AC_DEFINE([HAVE_LINUX_UUID], [1], [Build with support for Linux libuuid implementation])
    else
	$PKG_CONFIG --exists ossp-uuid
	if [[ $? -eq 0 ]]; then
	   have_libosspuuid=yes
	   LIBUUID_CFLAGS=$($PKG_CONFIG --cflags ossp-uuid)
	   LIBUUID_LIBS=$($PKG_CONFIG --libs ossp-uuid)
	fi
    fi
;;
*-*-darwin*)
    # uuid in system library - man uuid_generate
    have_libuuid=yes
;;
*-*-mingw32* | *-*-windows | *-*-cygwin*)
# Windows not supported
;;
*)
esac

if test "x${have_libuuid}" = xyes; then
    AC_DEFINE([HAVE_LIBUUID], [1], [Build for libuuid implementation])
elif test "x${have_libosspuuid}" = xyes; then
    AC_DEFINE([HAVE_OSSP_UUID], [1], [Build for ossp-uuid implementation])
else
    AC_MSG_ERROR([Cannot find either libuuid or ossp-uuid libraries])
fi

if test "x${use_static_files}" == xyes; then
   AC_DEFINE([ALLOW_STATIC_FILES], [1], [Serve static files from user specified root])
fi
if test "x${use_static_files}" == xyes && test "x${use_directory_listing}" == xyes; then
   AC_DEFINE([ALLOW_DIRECTORY_LISTING], [1], [Allow directory listings])
fi
if test "x${use_keep_alive}" == xyes; then
   AC_DEFINE([ENABLE_KEEP_ALIVE], [1], [Enable HTTP keep alive])
fi

CXXFLAGS="${CXXFLAGS} -Wno-deprecated-declarations ${LIBPQXX_CFLAGS} ${LIBYAML_CFLAGS} ${LIBUUID_CFLAGS} ${BOOST_CPPFLAGS} ${LIBPUGIXML_CFLAGS} ${LIBGDAL_CFLAGS} ${LIBPROJ_CFLAGS} ${LIBCAIROMM_CFLAGS} ${LIBCMARK_CFLAGS} ${FINALCUT_CFLAGS}"
dnl CPPFLAGS="${CPPFLAGS} ${XXXXXX_CPPFLAGS}"
LIBS="${LIBS} ${LIBPQXX_LIBS} ${LIBYAML_LIBS} ${LIBUUID_LIBS} ${BOOST_LDFLAGS} ${BOOST_LOCALE_LIB} ${LIBPUGIXML_LIBS} ${LIBGDAL_LIBS} ${LIBPROJ_LIBS} ${LIBCAIROMM_LIBS} ${LIBCMARK_LIBS} ${FINALCUT_LIBS}"

# Checks for header files.
dnl echo "Host_os: ${host_os}"
dnl echo "Host: ${host}"
dnl
AC_HEADER_ASSERT
dnl The following headers check is just to ensure the headers have been
dnl installed in addition to the actual library
AC_CHECK_HEADERS([pqxx/config-public-compiler.h], [have_pqxx_h=yes])
case $host in
  *-*-linux-*)
	if test "x${have_pqxx_h}" != xyes; then
  	   AC_MSG_ERROR([Cannot find PostgreSQL C++ client header files, pqxx/config-public-compiler.h])
  	fi
	;;
  *-*-darwin*)
	if test "x${have_pqxx_h}" != xyes; then
  	   AC_MSG_WARN([Cannot find PostgreSQL C++ client header files, pqxx/config-public-compiler.h])
  	fi
	;;
  *-*-mingw32* | *-*-windows | *-*-cygwin*)
        # Windows not supported
	;;
  *)
esac
dnl AC_CHECK_HEADERS([getopt.h uuid/uuid.h ossp/uuid.h])
dnl
dnl The presence of TargetConditionals.h indicates we're probably building
dnl using Xcode SDK so we can compile differently for macOS and iOS targets.
AC_CHECK_HEADERS([getopt.h TargetConditionals.h])
AC_CHECK_HEADERS([nlohmann/json.hpp],,
    [AC_MSG_ERROR([nlohmann/json.hpp not found.  Please install the nlohmann-json package.])])
AC_CHECK_HEADERS([thread])

# Check if the version of libuuid supports the uuid_generate_time_safe method
if test "x${have_libuuid}" = xyes; then
    SAFE_UUID_CHECK="#include <uuid/uuid.h>
      int main(int argc, char** argv) {
	uuid_t uuid;
	int safe = uuid_generate_time_safe(uuid);
      return 0;
    }"
    AC_MSG_CHECKING([if libuuid supports uuid_generate_time_safe])
    AC_LINK_IFELSE(
    [AC_LANG_SOURCE($SAFE_UUID_CHECK)],
    [AC_MSG_RESULT([yes]) AC_DEFINE([HAVE_LINUX_UUID], [1], [Build with support for Linux libuuid implementation])],
    [AC_MSG_RESULT([no])]
    )
fi

if test "x${use_boost_geometry}" = xyes; then
   AC_CHECK_HEADERS([boost/geometry.hpp boost/range.hpp boost/geometry/algorithms/simplify.hpp],,
	 [AC_MSG_ERROR([Boost library headers appear to be missing. Check config.log.])])
   AC_DEFINE([HAVE_BOOST_GEOMETRY], [1], [Build to use Boost Geometry headers])
   AC_DEFINE([BOOST_ALLOW_DEPRECATED_HEADERS], [1], [Suppress deprecated header warnings in Boost])
fi

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
