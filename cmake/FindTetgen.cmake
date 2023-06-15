# - Try to find tetgen
# Once done this will define
#  Tetgen_FOUND       - System has Tetgen
#  Tetgen_INSTALL_DIR - The tetgen install location

find_program(Tetgen_PATH tetgen)
get_filename_component(Tetgen_INSTALL_DIR ${Tetgen_PATH} DIRECTORY)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set Surreal_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
        Tetgen
        DEFAULT_MSG
        Tetgen_INSTALL_DIR
)

if (Tetgen_FOUND)
    message(STATUS "Tetgen Found: ${Tetgen_INSTALL_DIR}")
endif()

mark_as_advanced(Tetgen_INSTALL_DIR)