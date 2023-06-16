
set(headers distance_calculator.h)
set(LIBRARY_NAME distance_calculator)
add_library(${LIBRARY_NAME} STATIC distance_calculator.cpp ${headers})
target_link_libraries(${LIBRARY_NAME} PUBLIC parfait::parfait infinity::infinity)
set_standard_ring_rpath(${LIBRARY_NAME})
install(TARGETS ${LIBRARY_NAME} EXPORT infinity-targets DESTINATION lib)

set(EXECUTABLE_NAME ParallelDistanceCalculator)
add_executable(${EXECUTABLE_NAME} DistanceCalculatorUtility.cpp)
target_link_libraries(${EXECUTABLE_NAME} PUBLIC infinity::infinity ${LIBRARY_NAME})
set_standard_ring_rpath(${EXECUTABLE_NAME})
ring_install_executable(TARGETS ${EXECUTABLE_NAME} EXPORT infinity-targets DESTINATION bin)

install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
