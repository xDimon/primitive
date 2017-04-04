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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

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

# Подключаем spdlog
find_package(Spdlog)
if (NOT SPDLOG_FOUND)
    message(FATAL_ERROR "Spdlog not found")
endif ()
include_directories(${SPDLOG_INCLUDE_DIR})

# Подключаем openssl
find_package(OpenSSL)
if (NOT OPENSSL_FOUND)
    message(FATAL_ERROR "ssl not found")
endif ()
include_directories(${OPENSSL_INCLUDE_DIR})
list(APPEND "${CODENAME}_LIBRARIES" ${OPENSSL_LIBRARIES})

# Подключаем многопоточность
list(APPEND "${CODENAME}_LIBRARIES" pthread)


add_subdirectory(src)

list(APPEND SOURCE_FILES test/main.cpp)

add_executable(${CODENAME} ${SOURCE_FILES})

target_link_libraries(${CODENAME} ${${CODENAME}_LIBRARIES})
