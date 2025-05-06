# Determines where to install systemd service file

# SYSTEMD_CONF
# ------------
#
# SYSTEMD_CONF
#
# Include the following sections in the top level Makefile.am (replacing
# ${MYSERVICE_FILE} with the path to the relevant service file:
#
# AM_DISTCHECK_CONFIGURE_FLAGS = \
#	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)
#
# install-data-hook:
# if HAVE_SYSTEMD
#     systemdsystemunit_DATA = \
#     ${MYSERVICE}.service
# endif
#
#
# From daemon(7) man page

AC_DEFUN([SYSTEMD_CONF],
[
PKG_PROG_PKG_CONFIG()
AC_ARG_WITH([systemdsystemunitdir],
    [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],,
    [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
    def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

    AS_IF([test "x$def_systemdsystemunitdir" = "x"],
	[AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
	    [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
	    with_systemdsystemunitdir=no],
	    [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
    [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])
])
