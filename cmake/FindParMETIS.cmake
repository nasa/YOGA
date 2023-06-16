# - Try to find parmetis
# Once done this will define
#  PARMETIS_FOUND - System has PARMETIS
#  PARMETIS_INCLUDE_DIRS - The PARMETIS include directories
#  PARMETIS_LIBRARIES - The libraries needed to use PARMETIS
#  PARMETIS_DEFINITIONS - Compiler switches required for using PARMETIS

find_package(METIS)
if(METIS_FOUND)

    set(_PREFIX "${PARMETIS_PREFIX_DEFAULT}" CACHE STRING "Zoltan install directory")
    if(PARMETIS_PREFIX)
        message(STATUS "PARMETIS_PREFIX ${PARMETIS_PREFIX}")
    endif()

    find_path(PARMETIS_INCLUDE_DIR parmetis.h PATHS "${PARMETIS_PREFIX}/include")

    find_library(PARMETIS_LIBRARY parmetis PATHS "${PARMETIS_PREFIX}/lib")

    set(PARMETIS_LIBRARIES ${PARMETIS_LIBRARY} ${METIS_LIBRARIES})
    set(PARMETIS_INCLUDE_DIRS ${PARMETIS_INCLUDE_DIR} ${METIS_INCLUDE_DIRS})

    include(FindPackageHandleStandardArgs)
    # handle the QUIETLY and REQUIRED arguments and set PARMETIS_FOUND to TRUE
    # if all listed variables are TRUE
    find_package_handle_standard_args(
            ParMETIS
            DEFAULT_MSG
            PARMETIS_LIBRARY PARMETIS_INCLUDE_DIR
    )

    mark_as_advanced(PARMETIS_INCLUDE_DIR PARMETIS_LIBRARY)
    if(PARMETIS_FOUND AND NOT TARGET ParMETIS::ParMETIS)
        add_library(ParMETIS::ParMETIS UNKNOWN IMPORTED)
        set_target_properties(ParMETIS::ParMETIS PROPERTIES
                IMPORTED_LOCATION ${PARMETIS_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES ${PARMETIS_INCLUDE_DIR}
                )
        target_link_libraries(ParMETIS::ParMETIS INTERFACE METIS::METIS)
    endif()
else()
    set(PARMETIS_FOUND FALSE)
endif()


