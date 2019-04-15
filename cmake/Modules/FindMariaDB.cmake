# - Find mysqlclient
# Find the native MySQL includes and library
#
#  MARIADB_INCLUDE_DIR - where to find mysql.h, etc.
#  MARIADB_LIBRARIES   - List of libraries when using MySQL.
#  MARIADB_FOUND       - True if MySQL found.

if (MARIADB_INCLUDE_DIR)
    # Already in cache, be silent
    set(MARIADB_FIND_QUIETLY TRUE)
endif (MARIADB_INCLUDE_DIR)

find_path(MARIADB_INCLUDE_DIR mysql.h /usr/local/include/mariadb /usr/include/mariadb /usr/local/include/mysql /usr/include/mysql)

set(MARIADB_NAMES mariadbclient mariadbclient_r)
find_library(MARIADB_LIBRARY NAMES ${MARIADB_NAMES} PATHS /usr/lib /usr/local/lib PATH_SUFFIXES mariadb)

if (MARIADB_INCLUDE_DIR AND MARIADB_LIBRARY)
    set(MARIADB_FOUND TRUE)
    set(MARIADB_LIBRARIES ${MARIADB_LIBRARY})
else ()
    set(MARIADB_FOUND FALSE)
    set(MARIADB_LIBRARIES)
endif ()

if (MARIADB_FOUND)
    if (NOT MARIADB_FIND_QUIETLY)
        message(STATUS "Found MySQL: ${MARIADB_LIBRARY}")
    endif ()
else ()
    if (MARIADB_FIND_REQUIRED)
        message(STATUS "Looked for MySQL libraries named ${MARIADB_NAMES}.")
        message(FATAL_ERROR "Could NOT find MySQL library")
    endif ()
endif ()

mark_as_advanced(MARIADB_LIBRARY MARIADB_INCLUDE_DIR)
