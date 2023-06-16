include(CMakeFindDependencyMacro)
find_dependency(MessagePasser)
find_dependency(parfait)
find_dependency(tracer)

include("${CMAKE_CURRENT_LIST_DIR}/_infinity_interfaces.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/_infinity_base.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/_infinity_infinity.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/_infinity_infinity_static.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/_infinity_plugins.cmake")

