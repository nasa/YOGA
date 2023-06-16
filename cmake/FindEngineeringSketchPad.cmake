# - Try to find EngineeringSketchPad
# Once done this will define
#  EngineeringSketchPad_FOUND - System has EngineeringSketchPad

find_program(serveESP_PATH serveESP)
get_filename_component(EngineeringSketchPad_BIN_DIR ${serveESP_PATH} DIRECTORY)
find_library(CSM_LIB_PATH ocsm)
get_filename_component(EngineeringSketchPad_LIB_DIR ${CSM_LIB_PATH} DIRECTORY)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EngineeringSketchPad_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
        EngineeringSketchPad
        DEFAULT_MSG
        EngineeringSketchPad_BIN_DIR
        EngineeringSketchPad_LIB_DIR
)

if (EngineeringSketchPad_FOUND)
    message(STATUS "EngineeringSketchPad Found: ${EngineeringSketchPad_BIN_DIR};${EngineeringSketchPad_LIB_DIR}")
endif()

mark_as_advanced(EngineeringSketchPad_BIN_DIR)

