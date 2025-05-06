# TRIP_CHECK_UUID_LIBS
# --------------------
#
# Note comments in PostgreSQL documents stating OSSP is not well maintained.
# https://www.postgresql.org/docs/current/uuid-ossp.html
#
# On some platforms pkg-config is provided as 'ossp-uuid' and MacPorts as 'uuid'

# TRIP_CHECK_UUID_FUNC
# --------------------
# Check if the version of libuuid supports the uuid_generate_time_safe method

# Author:   Frank Dean <frank@fdsd.co.uk>
# Modified: 12-Mar-2025
# License:  MIT No Attribution https://github.com/aws/mit-0

AC_DEFUN([TRIP_CHECK_UUID_LIBS],
    [PKG_CHECK_MODULES([LIBUUID], [uuid],,
	[PKG_CHECK_MODULES([LIBUUID], [ossp-uuid],,
	    dnl Has to be a warning as the library is not needed on macOS
	    AC_MSG_WARN([No library for uuid found])
	    )]
	)
    AC_CHECK_HEADERS([uuid/uuid.h ossp/uuid.h],,,[@%:@include <string>])
# Check we have at least one of the headers defined
AS_IF([test x$ac_cv_header_uuid_uuid_h != xyes && test x$ac_cv_header_ossp_uuid_h != xyes],
   [AC_MSG_ERROR([Cannot find either uuid/uuid.h or ossp/uuid.h])])
])

# Check if the version of libuuid supports the uuid_generate_time_safe method
AC_DEFUN([TRIP_CHECK_UUID_FUNC],
    [AS_IF([test "x$ac_cv_header_uuid_uuid_h" = xyes], [
	AC_MSG_CHECKING([if libuuid supports uuid_generate_time_safe])
	AC_LINK_IFELSE(
	    [AC_LANG_PROGRAM([[@%:@include <uuid/uuid.h>]],
	    [[  uuid_t uuid;
  int safe = uuid_generate_time_safe(uuid);]])],
	    [AC_MSG_RESULT([yes]) AC_DEFINE([HAVE_SAFE_UUID], [1], [Build with support for time safe UUID generation with libuuid])],
	    [AC_MSG_RESULT([no])]
	    )
])])
