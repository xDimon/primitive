file(GLOB_RECURSE src_files *.cpp)

#message(STATUS "Found source files:")
#foreach(F ${src_files})
#    message(STATUS "  ${F}")
#endforeach()

list(APPEND SOURCE_FILES ${src_files})

set(SOURCE_FILES "${SOURCE_FILES}" PARENT_SCOPE)
