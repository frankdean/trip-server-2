# Provides options to enable static files, directory listings and keep alive

# TRIP_ARG_ENABLE
# ---------------
#
# TRIP_ARG_ENABLE(default, feature, enable-help-string, disable-help-string, description)
#
# Where:
# default: either 'yes' for enabled by default or 'no' otherwise
# feature: is passed as the feature name to AC_ARG_ENABLE
# enable-help-string: used as AC_ARG_ENABLE help string when default is not 'yes'
# disable-help-string: used as AC_ARG_ENABLE help string when default is 'yes'
# description: passed as description to AC_DEFINE
#
# The variable name passed to AC_DEFINE is the 'feature' parameter prefixed
# with 'ENABLE_' in upper case with hyphens replaced with underscore.
#
#
# TRIP_WEB_OPTION_STATIC_FILES
# ----------------------------
#
# Takes a single parameter of either 'yes' or 'no' specifying whether the
# static-files option should be enabled by default.
# Calls TRIP_ARG_ENABLE with pre-defined values.
#
#
# TRIP_WEB_OPTION_DIRECTORY_LISTING
# ---------------------------------
# Takes a single parameter of either 'yes' or 'no' specifying whether the
# directory-listing option should be enabled by default.
# Calls TRIP_ARG_ENABLE with pre-defined values.
#
#
# TRIP_WEB_OPTION_KEEP_ALIVE
# --------------------------
#
# Takes a single parameter of either 'yes' or 'no' specifying whether the
# keep-alive option should be enabled by default.
# Calls TRIP_ARG_ENABLE with pre-defined values.
#
#
# Author:   Frank Dean <frank@fdsd.co.uk>
# Modified: 14-Mar-2025
# License:  MIT No Attribution https://github.com/aws/mit-0

AC_DEFUN([TRIP_ARG_ENABLE],
[
define([feature_string],
    [dnl
"x$enable_[]translit([[$2]], [-a-z], [_a-z])"])dnl
define([enable_help_string],
    [dnl
--enable-[][$2]])dnl
define([disable_help_string],
    [dnl
--disable-[][$2]])dnl
define([defined_value],
    [dnl
ENABLE_[]translit([[$2]], [-a-z], [_A-Z])])dnl
AC_ARG_ENABLE([$2], [ifelse([$1], [yes],
    [AS_HELP_STRING([disable_help_string], [$4])],
    [AS_HELP_STRING([enable_help_string], [$3])])])
AS_IF([test feature_string = xyes || (test feature_string != xno && test "x$1" = xyes)],
   [AC_DEFINE([defined_value], [1], [$5])])
])

AC_DEFUN([TRIP_WEB_OPTION_STATIC_FILES],
[
TRIP_ARG_ENABLE([$1],
    [static-files],
    [Enable serving static files],
    [Disable serving static files],
    [Serve static files from user specified root])])

AC_DEFUN([TRIP_WEB_OPTION_DIRECTORY_LISTING],
[
TRIP_ARG_ENABLE([$1],
    [directory-listing],
    [Enable HTTP directory listing],
    [Disable HTTP directory listing],
    [Allow directory listings])])

AC_DEFUN([TRIP_WEB_OPTION_KEEP_ALIVE],
[
TRIP_ARG_ENABLE([$1],
    [keep-alive],
    [Enables HTTP keep alive],
    [Disables HTTP keep alive],
    [Enable HTTP keep alive])])
