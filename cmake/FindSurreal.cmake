# - Try to find surreal
# Once done this will define
#  Surreal_FOUND - System has Surreal
#  Surreal_INCLUDE_DIRS - The surreal include directories

set(_PREFIX "${Surreal_PREFIX_DEFAULT}" CACHE STRING "Surreal directory")
if(Surreal_PREFIX)
    message(STATUS "Surreal_PREFIX ${Surreal_PREFIX}")
endif()

find_path(Surreal_INCLUDE_DIR timer.h PATHS "${Surreal_PREFIX}/src")

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set Surreal_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
        Surreal
        DEFAULT_MSG
        Surreal_INCLUDE_DIR
)

mark_as_advanced(Surreal_INCLUDE_DIR)