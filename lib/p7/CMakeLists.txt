cmake_minimum_required(VERSION 2.8)
project(p7 CXX)

set(VERSION "4.2")

file(GLOB_RECURSE src_files Sources/*.?pp)

#message(STATUS "P7 include dirs:")
#foreach(F ${P7_AUX_INCLUDE_DIRS})
#    message(STATUS "  ${F}")
#endforeach()

list(APPEND "P7_AUX_INCLUDE_DIRS" "${CMAKE_CURRENT_SOURCE_DIR}/Shared/")
if (UNIX)
    list(APPEND "P7_AUX_INCLUDE_DIRS" "${CMAKE_CURRENT_SOURCE_DIR}/Shared/Platforms/Linux_x86/")
endif ()

add_library(p7 ${src_files})

#set_target_properties(p7 PROPERTIES LINKER_LANGUAGE CXX)
#set_target_properties(p7 PROPERTIES COMPILE_FLAGS "-fPIC")
#target_sources(p7 PRIVATE ${src_files})
#target_include_directories(p7 PRIVATE "${P7_AUX_INCLUDE_DIRS}")
#target_include_directories(p7 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Headers/")
#target_link_libraries(p7 PRIVATE pthread rt)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Headers/")

set_property(TARGET p7 APPEND PROPERTY INCLUDE_DIRECTORIES "${P7_AUX_INCLUDE_DIRS}")

target_link_libraries(p7 pthread rt)

set(P7_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Headers" PARENT_SCOPE)
set(P7_LIBRARIES p7 PARENT_SCOPE)
set(P7_LIBRARY p7 PARENT_SCOPE)
set(P7_FOUND TRUE PARENT_SCOPE)
