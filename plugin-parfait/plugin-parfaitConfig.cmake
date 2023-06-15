include(CMakeFindDependencyMacro)
find_dependency(t-infinity)

include("${CMAKE_CURRENT_LIST_DIR}/plugin_parfait_core.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/S_PreProcessor.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/ParfaitViz.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/NC_PreProcessor.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CC_ReProcessor.cmake")
