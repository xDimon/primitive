file(GLOB_RECURSE src_files *.cpp)

#messages(STATUS "Found source files:")
#foreach(F ${src_files})
#    messages(STATUS "  ${F}")
#endforeach()

list(APPEND SOURCE_FILES ${src_files})

set(SOURCE_FILES "${SOURCE_FILES}" PARENT_SCOPE)
