bin_PROGRAMS = trip-server
trip_server_SOURCES = \
	download_triplogger_configuration_handler.cpp \
	download_triplogger_configuration_handler.hpp \
	geo_utils.cpp \
	geo_utils.hpp \
	main.cpp \
	session_pg_dao.cpp \
	session_pg_dao.hpp \
	tile_handler.cpp \
	tile_handler.hpp \
	tile_pg_dao.cpp \
	tile_pg_dao.hpp \
	track_logging_handler.cpp \
	track_logging_handler.hpp \
	track_sharing_edit_handler.cpp \
	track_sharing_edit_handler.hpp \
	track_sharing_handler.cpp \
	track_sharing_handler.hpp \
	tracking_download_handler.cpp \
	tracking_download_handler.hpp \
	tracking_info_handler.cpp \
	tracking_info_handler.hpp \
	tracking_map_handler.cpp \
	tracking_map_handler.hpp \
	tracking_pg_dao.cpp \
	tracking_pg_dao.hpp \
	tracking_request_handler.cpp \
	tracking_request_handler.hpp \
	tracking_rest_handler.cpp \
	tracking_rest_handler.hpp \
	trip_application.cpp \
	trip_application.hpp \
	trip_config.cpp \
	trip_config.hpp \
	trip_css_handler.cpp \
	trip_css_handler.hpp \
	trip_get_options.cpp \
	trip_get_options.hpp \
	trip_pg_dao.cpp \
	trip_pg_dao.hpp \
	trip_request_factory.cpp \
	trip_request_factory.hpp \
	trip_request_handler.cpp \
	trip_request_handler.hpp \
	trip_session_manager.cpp \
	trip_session_manager.hpp \
	../trip-server-common/src/application.cpp \
	../trip-server-common/src/application.hpp \
	../trip-server-common/src/configuration.cpp \
	../trip-server-common/src/configuration.hpp \
	../trip-server-common/src/dao_helper.cpp \
	../trip-server-common/src/dao_helper.hpp \
	../trip-server-common/src/date_utils.cpp \
	../trip-server-common/src/date_utils.hpp \
	../trip-server-common/src/debug_utils.cpp \
	../trip-server-common/src/debug_utils.hpp \
	../trip-server-common/src/file_utils.cpp \
	../trip-server-common/src/file_utils.hpp \
	../trip-server-common/src/get_options.cpp \
	../trip-server-common/src/get_options.hpp \
	../trip-server-common/src/http_client.cpp \
	../trip-server-common/src/http_client.hpp \
	../trip-server-common/src/http_request.cpp \
	../trip-server-common/src/http_request.hpp \
	../trip-server-common/src/http_request_factory.cpp \
	../trip-server-common/src/http_request_factory.hpp \
	../trip-server-common/src/http_request_handler.cpp \
	../trip-server-common/src/http_request_handler.hpp \
	../trip-server-common/src/http_response.cpp \
	../trip-server-common/src/http_response.hpp \
	../trip-server-common/src/logger.cpp \
	../trip-server-common/src/logger.hpp \
	../trip-server-common/src/pagination.cpp \
	../trip-server-common/src/pagination.hpp \
	../trip-server-common/src/pg_pool.cpp \
	../trip-server-common/src/pg_pool.hpp \
	../trip-server-common/src/session.cpp \
	../trip-server-common/src/session.hpp \
	../trip-server-common/src/socket.cpp \
	../trip-server-common/src/socket.hpp \
	../trip-server-common/src/uri_utils.cpp \
	../trip-server-common/src/uri_utils.hpp \
	../trip-server-common/src/uuid.cpp \
	../trip-server-common/src/uuid.hpp \
	../trip-server-common/src/worker.cpp \
	../trip-server-common/src/worker.hpp

check_PROGRAMS = test_uuid test_dao_helper test_date_utils test_file_utils \
	test_uri_utils test_debug_utils test_pagination test_pool \
	test_tracking test_geo_utils
test_uuid_SOURCES = ../trip-server-common/src/test_uuid.cpp
test_dao_helper_SOURCES = ../trip-server-common/src/test_dao_helper.cpp \
	../trip-server-common/src/date_utils.cpp \
	../trip-server-common/src/date_utils.hpp \
	../trip-server-common/src/logger.cpp \
	../trip-server-common/src/logger.hpp
test_date_utils_SOURCES = ../trip-server-common/src/test_date_utils.cpp \
	../trip-server-common/src/logger.cpp \
	../trip-server-common/src/logger.hpp
test_file_utils_SOURCES = ../trip-server-common/src/test_file_utils.cpp \
	../trip-server-common/src/date_utils.cpp \
	../trip-server-common/src/date_utils.hpp \
	../trip-server-common/src/logger.cpp \
	../trip-server-common/src/logger.hpp
test_uri_utils_SOURCES = ../trip-server-common/src/test_uri_utils.cpp
test_debug_utils_SOURCES = ../trip-server-common/src/test_debug_utils.cpp
test_pagination_SOURCES = ../trip-server-common/src/test_pagination.cpp \
	../trip-server-common/src/logger.cpp \
	../trip-server-common/src/logger.hpp \
	../trip-server-common/src/debug_utils.cpp \
	../trip-server-common/src/debug_utils.hpp
test_pool_SOURCES = ../trip-server-common/src/test_pool.cpp
test_tracking_SOURCES = test_tracking.cpp \
	geo_utils.cpp \
	../trip-server-common/src/logger.cpp \
	../trip-server-common/src/logger.hpp
test_geo_utils_SOURCES = test_geo_utils.cpp

AM_CPPFLAGS = -DDATADIR='"$(datadir)"' -DSYSCONFDIR='"$(sysconfdir)"'
