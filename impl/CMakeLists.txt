file(GLOB_RECURSE src_files main.cpp)
list(APPEND SOURCE_FILES ${src_files})

file(GLOB_RECURSE src_files *.cpp)
list(APPEND SOURCE_FILES ${src_files})

#message(STATUS "Found source files:")
#foreach(F ${src_files})
#    message(STATUS "  ${F}")
#endforeach()

set(SOURCE_FILES "${SOURCE_FILES}" PARENT_SCOPE)
