cmake_minimum_required(VERSION 3.5)

# Имя проекта
set(CODENAME "primitive")
project(${CODENAME})

# Запрещаем сборку не на линукс
if (NOT UNIX)
	message(FATAL_ERROR "Not Unix!")
endif ()

# Подключаем внешние модули cmake
list(APPEND CMAKE_MODULE_PATH /usr/share/cmake-3.5/Modules/)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/)

message(STATUS "Module paths:")
foreach(F ${CMAKE_MODULE_PATH})
    message(STATUS "  ${F}")
endforeach()


# Включаем использование ccache, если установлен
find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
else ()
    message(STATUS "Try to install `ccache` for faster compilation")
endif ()

# Задаем стандарт С++14
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -rdynamic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -rdynamic")

set(${CODENAME}_LIBRARIES)

# Подключаем cxxopts
include_directories(lib/cxxopts/include)

# Подключаем libconfig++
find_package(Libconfig++)
if (NOT LIBCONFIG++_FOUND)
    message(FATAL_ERROR "libconfig++ not found")
endif ()
include_directories(${LIBCONFIG++_INCLUDE_DIR})
list(APPEND "${CODENAME}_LIBRARIES" ${LIBCONFIG++_LIBRARIES})

# Подключаем p7
find_package(P7)
if (NOT P7_FOUND)
    add_subdirectory(lib/p7)
endif ()
include_directories(${P7_INCLUDE_DIR})
list(APPEND "${CODENAME}_LIBRARIES" ${P7_LIBRARIES})

# Подключаем openssl
find_package(OpenSSL)
if (NOT OPENSSL_FOUND)
    message(FATAL_ERROR "ssl not found")
endif ()
include_directories(${OPENSSL_INCLUDE_DIR})
list(APPEND "${CODENAME}_LIBRARIES" ${OPENSSL_LIBRARIES})

# Подключаем многопоточность
list(APPEND "${CODENAME}_LIBRARIES" pthread)


# Подключаем mysql
find_package(MySQL)
if (MYSQL_FOUND)
    include_directories(${MYSQL_INCLUDE_DIR})
    list(APPEND "${CODENAME}_LIBRARIES" ${MYSQL_LIBRARIES})
else()
    message(FATAL "libmysqlclient not found")
endif ()

# Подключаем libconfig++
find_package(Libconfig++)
if (LIBCONFIG++_FOUND)
    include_directories(${LIBCONFIG++_INCLUDE_DIR})
    list(APPEND "${CODENAME}_LIBRARIES" ${LIBCONFIG++_LIBRARIES})
else()
    message(FATAL "libconfig++ not found")
endif ()

list(APPEND "${CODENAME}_LIBRARIES" dl)

message(STATUS "Project libraries:")
foreach(F ${${CODENAME}_LIBRARIES})
	message(STATUS "  ${F}")
endforeach()

add_subdirectory(src)

list(APPEND SOURCE_FILES test/main.cpp)

add_executable(${CODENAME} ${SOURCE_FILES})

target_link_libraries(${CODENAME} ${${CODENAME}_LIBRARIES})
