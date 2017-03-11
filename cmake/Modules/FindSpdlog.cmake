find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h
    /usr/include
    /usr/local/include
)

find_library(SPDLOG_LIBRARY
    NAMES
        spdlog
    PATH
        /usr/lib
        /usr/local/lib
)

if (SPDLOG_INCLUDE_DIR
        #AND SPDLOG_LIBRARY
        )
	set(SPDLOG_FOUND TRUE)
	set(SPDLOG_INCLUDE_DIRS  ${SPDLOG_INCLUDE_DIR})
#	set(SPDLOG_LIBRARIES  ${SPDLOG_LIBRARY})
endif ()

if (SPDLOG_FOUND)
#	if (NOT SPDLOG_FIND_QUIETLY)
#		message(STATUS "Found spdlog: ${SPDLOG_LIBRARY}")
#	endif ()
else ()
	if (Spdlog_FIND_REQUIRED)
		if (NOT SPDLOG_INCLUDE_DIR)
			message(FATAL_ERROR "Could not find spdlog header file!")
		endif ()

#		if (NOT SPDLOG_LIBRARY)
#			message(FATAL_ERROR "Could not find spdlog library file!")
#		endif ()
	endif ()
endif ()
