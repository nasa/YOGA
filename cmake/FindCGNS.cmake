# - Try to find cgns
# Once done this will define
#  CGNS_FOUND - System has CGNS
#  CGNS_INCLUDE_DIRS - The CGNS include directories
#  CGNS_LIBRARIES - The libraries needed to use CGNS
#  CGNS_DEFINITIONS - Compiler switches required for using CGNS

set(_PREFIX "${CGNS_PREFIX_DEFAULT}" CACHE STRING "CGNS install directory")
if(CGNS_PREFIX)
    message(STATUS "CGNS_PREFIX ${CGNS_PREFIX}")
endif()

find_path(CGNS_INCLUDE_DIR cgnslib.h PATHS "${CGNS_PREFIX}/include")

find_library(CGNS_LIBRARY cgns PATHS "${CGNS_PREFIX}/lib")

set(CGNS_LIBRARIES ${CGNS_LIBRARY} )
set(CGNS_INCLUDE_DIRS ${CGNS_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CGNS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
        CGNS
        DEFAULT_MSG
        CGNS_LIBRARY CGNS_INCLUDE_DIR
)

mark_as_advanced(CGNS_INCLUDE_DIR CGNS_LIBRARY )

if(CGNS_FOUND AND NOT TARGET CGNS::CGNS)
    add_library(CGNS::CGNS UNKNOWN IMPORTED)
    set_target_properties(CGNS::CGNS PROPERTIES
            IMPORTED_LOCATION ${CGNS_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${CGNS_INCLUDE_DIR}
            )
endif()
