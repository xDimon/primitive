cmake_minimum_required(VERSION 2.8)

# Имя проекта
project("primitive")

set(CODENAME "primitive")

string(TOUPPER ${CODENAME} CODENAMEUC)

# Запрещаем сборку не на линукс
if (NOT UNIX)
	message(FATAL_ERROR "Not Unix!")
endif ()

get_directory_property(hasParent PARENT_DIRECTORY)

# Подключаем внешние модули cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/)

#message(STATUS "Module paths:")
#foreach(F ${CMAKE_MODULE_PATH})
#    message(STATUS "  ${F}")
#endforeach()

# Включаем использование ccache, если установлен
find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
else ()
    message(STATUS "Try to install `ccache` for faster compilation")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -rdynamic -O0 -g")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -rdynamic -O0 -g")

if (NOT PROJECT_NAME)
    set(PROJECT_NAME ${CODENAME})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPROJECT_NAME=${PROJECT_NAME}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROJECT_NAME=${PROJECT_NAME}")
endif ()

#set(SANITIZERS "-fsanitize=leak")
#set(SANITIZERS "-fsanitize=address")
#set(SANITIZERS "-fsanitize=thread")
#set(SANITIZERS "-fsanitize=undefined")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZERS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZERS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZERS}")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

set(${CODENAME}_INCLUDES)
set(${CODENAME}_LIBRARIES)

# Подключаем динамическую линковку
list(APPEND "${CODENAME}_LIBRARIES" dl)

# Подключаем многопоточность
find_package(Threads REQUIRED)
if (NOT THREADS_FOUND)
    message(FATAL_ERROR "library for multithreading not found")
endif ()
list(APPEND "${CODENAME}_LIBRARIES" ${CMAKE_THREAD_LIBS_INIT})

# Подключаем cxxopts
list(APPEND "${CODENAME}_INCLUDES" ${CMAKE_CURRENT_SOURCE_DIR}/lib/cxxopts/include)

# Подключаем libconfig++
find_package(Libconfig++)
if (NOT LIBCONFIG++_FOUND)
    message(FATAL_ERROR "libconfig++ not found")
endif ()
list(APPEND "${CODENAME}_INCLUDES" ${LIBCONFIG++_INCLUDE_DIR})
list(APPEND "${CODENAME}_LIBRARIES" ${LIBCONFIG++_LIBRARIES})

# Подключаем p7
find_package(P7)
if (NOT P7_FOUND)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/p7)
endif ()
list(APPEND "${CODENAME}_INCLUDES" ${P7_INCLUDE_DIR})
list(APPEND "${CODENAME}_LIBRARIES" ${P7_LIBRARIES})

# Подключаем openssl
find_package(OpenSSL)
if (NOT OPENSSL_FOUND)
    message(FATAL_ERROR "ssl not found")
endif ()
list(APPEND "${CODENAME}_INCLUDES" ${OPENSSL_INCLUDE_DIR})
list(APPEND "${CODENAME}_LIBRARIES" ${OPENSSL_LIBRARIES})

# Подключаем mysql
find_package(MySQL)
if (NOT MYSQL_FOUND)
    message(FATAL_ERROR "libmysqlclient not found")
endif ()
list(APPEND "${CODENAME}_INCLUDES" ${MYSQL_INCLUDE_DIR})
list(APPEND "${CODENAME}_LIBRARIES" ${MYSQL_LIBRARIES})

# Подключаем protobuf++
find_package(Protobuf REQUIRED)
if (NOT PROTOBUF_FOUND)
    message(FATAL_ERROR "Protobuf not found")
endif ()
list(APPEND "${CODENAME}_INCLUDES" ${PROTOBUF_INCLUDE_DIRS})
list(APPEND "${CODENAME}_LIBRARIES" ${PROTOBUF_LIBRARIES})

#message(STATUS "Project includes:")
foreach(F ${${CODENAME}_INCLUDES})
#	message(STATUS "  ${F}")
	include_directories(${F})
endforeach()

#message(STATUS "Project libraries:")
#foreach(F ${${CODENAME}_LIBRARIES})
#	message(STATUS "  ${F}")
#endforeach()

add_subdirectory(src)
add_subdirectory(impl)

add_library(${CODENAME}_object OBJECT ${SOURCE_FILES})
set_target_properties(${CODENAME}_object PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(${CODENAME}_shared SHARED $<TARGET_OBJECTS:${CODENAME}_object>)
target_link_libraries(${CODENAME}_shared ${${CODENAME}_LIBRARIES})
set_target_properties(${CODENAME}_shared PROPERTIES OUTPUT_NAME ${CODENAME})
add_dependencies(${CODENAME}_shared ${CODENAME}_object)

add_library(${CODENAME}_static STATIC $<TARGET_OBJECTS:${CODENAME}_object>)
target_link_libraries(${CODENAME}_static ${${CODENAME}_LIBRARIES})
set_target_properties(${CODENAME}_static PROPERTIES OUTPUT_NAME ${CODENAME})
add_dependencies(${CODENAME}_static ${CODENAME}_object)

add_executable(${CODENAME}_executable $<TARGET_OBJECTS:${CODENAME}_object>)
target_link_libraries(${CODENAME}_executable ${${CODENAME}_LIBRARIES})
set_target_properties(${CODENAME}_executable PROPERTIES OUTPUT_NAME ${CODENAME})
add_dependencies(${CODENAME}_executable ${CODENAME}_object)

if(hasParent)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
    set(${CODENAMEUC}_INCLUDES ${${CODENAME}_INCLUDES} PARENT_SCOPE)
    set(${CODENAMEUC}_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" PARENT_SCOPE)
    set(${CODENAMEUC}_LIBRARIES "${${CODENAME}_LIBRARIES}" PARENT_SCOPE)
    set(${CODENAMEUC}_LIBRARY ${CODENAME} PARENT_SCOPE)
    set(${CODENAMEUC}_OBJECTS $<TARGET_OBJECTS:${CODENAME}_object> PARENT_SCOPE)
    set(${CODENAMEUC}_FOUND TRUE PARENT_SCOPE)
endif()
