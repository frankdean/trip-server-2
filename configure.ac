#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([trip],[2.0.0-alpha.38],[frank.dean@fdsd.co.uk])
BOOTSTRAP_VERSION='5.2.3'
OPENLAYERS_VERSION='7.3.0'
PROJ4JS_VERSION='2.9.0'
OLC_VERSION='1.0.4'
ZXCVBN_VERSION='4.4.2'
AC_COPYRIGHT([Copyright (C) 2022 Frank Dean])
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-xz subdir-objects info-in-builddir])
AC_CONFIG_SRCDIR([src/trip_request_factory.cpp])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_TESTDIR([tests])

dnl https://www.gnu.org/software/automake/manual/automake.html#maintainer_002dmode
AM_MAINTAINER_MODE([enable])

AC_ARG_ENABLE([keep-alive],
    AS_HELP_STRING([--enable-keep-alive],
	[Build to enable HTTP keep-alive (default no)]),
	[use_keep_alive=${enableval}],
	[use_keep_alive=no])
AC_ARG_ENABLE([static-files],
    AS_HELP_STRING([--enable-static-files],
	[Build to allow serving of static files (default yes)]),
	[use_static_files=${enableval}],
	[use_static_files=yes])
AC_ARG_ENABLE([directory-listing],
    AS_HELP_STRING([--enable-directory-listing],
	[Build to allow directory listings (default no)]),
	[use_directory_listing=${enableval}],
	[use_directory_listing=no])
AC_ARG_ENABLE([gdal],
    AS_HELP_STRING([--enable-gdal],
	[Build using GDAL library (default yes)]),
	[use_gdal=${enableval}],
	[use_gdal=yes])

# Checks for programs.
dnl : ${CXXFLAGS="-g -O0 -std=c++11"}
AX_CXX_COMPILE_STDCXX(11)
AC_PROG_CXX
AC_PROG_INSTALL
AC_LANG(C++)
AC_CHECK_PROGS([DOCBOOK2TEXI],[docbook2texi docbook2x-texi])
AC_CHECK_PROGS([DOCBOOK2MAN],[docbook2man docbook2x-man])

# Set $host variable
AC_CANONICAL_HOST

# Checks for libraries.
AC_SEARCH_LIBS([pthread_create], [pthread])
AX_BOOST_BASE([1.67],[],[AC_MSG_ERROR([Please install at least version 1.67 of the Boost Locale Library])])
AX_BOOST_LOCALE
PKG_CHECK_MODULES([LIBPQXX], [libpqxx >= 6.2.5 libpqxx < 7 libpq >= 11.14 yaml-cpp >= 0.6.2])
PKG_CHECK_MODULES([LIBPUGIXML], [pugixml >= 1.11])
if test "x${use_gdal}" = xyes; then
   PKG_CHECK_MODULES([LIBGDAL], [gdal >= 3.2.2])
   AC_DEFINE([HAVE_GDAL], [1], [Build to use GDAL library])
   dnl GDAL_SYSTEM='elevation_tile.$(OBJEXT)'
   dnl AC_SUBST([GDAL_SYSTEM])
fi
AM_CONDITIONAL([USE_GDAL], [test x$use_gdal = xyes])

# echo $host

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

CXXFLAGS="${CXXFLAGS} -Wsuggest-override -Wno-deprecated-declarations ${LIBPQXX_CFLAGS} ${LIBUUID_CFLAGS} ${BOOST_CPPFLAGS} ${LIBPUGIXML_CFLAGS} ${LIBGDAL_CFLAGS}"
dnl CPPFLAGS="${CPPFLAGS} ${XXXXXX_CPPFLAGS}"
LIBS="${LIBS} ${LIBPQXX_LIBS} ${LIBUUID_LIBS} ${BOOST_LDFLAGS} ${BOOST_LOCALE_LIB} ${LIBPUGIXML_LIBS} ${LIBGDAL_LIBS}"

# Checks for header files.
dnl echo "Host_os: ${host_os}"
dnl echo "Host: ${host}"
dnl
dnl The following headers check is just to ensure the headers have been
dnl installed in addition to the actual library
AC_HEADER_ASSERT
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
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([nlohmann/json.hpp],,
    [AC_MSG_ERROR([nlohmann/json.hpp not found.  Please install the nlohmann-json package.])])

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

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
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

AC_CONFIG_FILES([Makefile po/Makefile.in
	         doc/Makefile
                 src/Makefile
                 tests/Makefile
                 ])
AC_OUTPUT
