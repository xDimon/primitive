cmake_minimum_required(VERSION 3.6)

# Имя проекта
set(CODENAME "primitive")
project(${CODENAME})

# Запрещаем сборку не на линукс
if (NOT UNIX)
	message(FATAL_ERROR "Not Unix!")
endif ()

# Подключаем внешние модули cmake
list(APPEND CMAKE_MODULE_PATH /usr/share/cmake-?.?/Modules/)

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

set(SOURCE_FILES src/server/Server.cpp src/thread/ThreadPool.cpp)

list(APPEND SOURCE_FILES test/main.cpp)

add_executable(${CODENAME} ${SOURCE_FILES})

target_link_libraries(${CODENAME} pthread)
