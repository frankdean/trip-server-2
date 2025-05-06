# TRIP_CHECK_PQXX_BINARY_CAST
# ---------------------------
#
# Checks whether using binary_cast with libpqxx will compile.  If not,
# ENABLE_LIBPQXX_BINARYSTRING will be defined and the application should use
# the deprecated binarystring method of passing parameters to a PostgreSQL
# BYTEA column.
#
# The macro must be called after LIBS has been set, otherwise there will be
# linking errors.
#
# Author:   Frank Dean <frank@fdsd.co.uk>
# Modified: 19-Mar-2025
# License:  MIT No Attribution https://github.com/aws/mit-0

AC_DEFUN([TRIP_CHECK_PQXX_BINARY_CAST],
    [AS_IF([test "${have_libpqxx7}" -eq 1], [
        AC_MSG_CHECKING([if libpqxx supports binary_cast])
	AC_LINK_IFELSE(
	    [AC_LANG_PROGRAM([[@%:@include <pqxx/pqxx>]],
	    [[  pqxx::connection connection{""};
  pqxx::work tx(connection);
  pqxx::bytes_view b;
  auto r = tx.exec_params("SELECT * FROM (SELECT 1 AS xx) WHERE xx=$1::bytea", pqxx::binary_cast(b));
  auto bs = r[0]["image"].as<pqxx::bytes>();]])],
	    AC_MSG_RESULT([yes]),
 	    AC_MSG_RESULT([no]) AC_DEFINE([ENABLE_LIBPQXX_BINARYSTRING], [1], [Use deprecated binarystring])
	    AC_MSG_WARN([Link error using pqxx::binary_cast - libpqxx may have been compiled with options inconsistent with the options used to configure this application])
	)
])
])
