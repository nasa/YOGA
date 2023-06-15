# - Try to find MASA 
# Once done this will define
#  MASA_FOUND - System has MASA
#  MASA_INCLUDE_DIRS - The MASA include directories
#  MASA_LIBRARIES - The libraries needed to use MASA
#  MASA_DEFINITIONS - Compiler switches required for using MASA

find_path(MASA_INCLUDE_DIR masa.h PATHS "${MASA_PREFIX}/include")

find_library(MASA_LIBRARY masa PATHS "${MASA_PREFIX}/lib")

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set MASA_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
        masa
        DEFAULT_MSG
        MASA_LIBRARY MASA_INCLUDE_DIR
)

mark_as_advanced(MASA_INCLUDE_DIR MASA_LIBRARY)
if(MASA_FOUND AND NOT TARGET masa::masa)
  add_library(masa::masa UNKNOWN IMPORTED)
  set_target_properties(masa::masa PROPERTIES
    IMPORTED_LOCATION ${MASA_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${MASA_INCLUDE_DIR}
  ) 
endif()
  


