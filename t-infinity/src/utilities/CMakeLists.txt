
function(utility_install target_name destination)
    target_link_libraries(${target_name} PUBLIC infinity::infinity)
    set_standard_ring_rpath(${target_name} ../lib)
    ring_install_executable(TARGETS ${target_name} EXPORT infinity-targets DESTINATION ${destination})
endfunction()

add_subdirectory(DistanceCalculator)

set(LIBRARY_NAME CViz)
SET(VIZ_HEADERS CViz.h)
set(SOURCES ${VIZ_HEADERS} CViz.cpp)
add_library(${LIBRARY_NAME} STATIC ${SOURCES})
set_standard_ring_rpath(${LIBRARY_NAME})
target_include_directories(${LIBRARY_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../t-infinity-interfaces>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
        )
target_link_libraries(${LIBRARY_NAME} PUBLIC infinity::infinity)
install(FILES ${VIZ_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/)
install(TARGETS ${LIBRARY_NAME} EXPORT infinity-targets DESTINATION lib)

install(PROGRAMS
        interpolate.py
        plot.py
        plot-diff.py
        snap.py
        calchessian.py
        DESTINATION utils)

add_library(SnapReader SHARED SnapLoader.cpp)
add_library(infinity::SnapReader ALIAS SnapReader)
utility_install(SnapReader lib)

add_executable(inf inf.cpp)
target_compile_definitions(inf PRIVATE DRIVER_PREFIX="inf")
target_link_libraries(inf PUBLIC
        parfait::parfait
        infinity::infinity
        tracer::tracer
        )
set_standard_ring_rpath(inf)
ring_install_executable(TARGETS inf EXPORT infinity-targets DESTINATION bin)

add_subcommand(inf core extensions ExtensionsCommand.cpp)
add_subcommand(inf core csv-to-snap CsvToSnapCommand.cpp)
add_subcommand(inf core distance DistanceCommand.cpp)
add_subcommand(inf core plugins ListPluginsCommand.cpp)
add_subcommand(inf core plot PlotCommand.cpp)
add_subcommand(inf core cartmesh CreateCartMeshCommand.cpp)
add_subcommand(inf core transform TransformCommand.cpp)
add_subcommand(inf core validate ValidateCommand.cpp)
add_subcommand(inf core fix FixCommand.cpp)
add_subcommand(inf core snap SnapCommand.cpp)
add_subcommand(inf core mirror MirrorCommand.cpp)
add_subcommand(inf devtools stencil StencilCommand.cpp)
add_subcommand(inf devtools test-fields TestFieldsCommand.cpp)
add_subcommand(inf experimental mms MMSCommand.cpp)
add_subcommand(inf experimental opt MeshOptimizeCommand.cpp)
add_subcommand(inf experimental stitch StitchCommand.cpp)
add_subcommand(inf experimental elevate ElevateCommand.cpp)
add_subcommand(inf experimental merge MergeCommand.cpp)
#add_subcommand(fs-perf FilesystemPerformanceCommand.cpp)
add_subcommand(inf core interpolate InterpolateCommand.cpp)
add_subcommand(inf experimental oml-translate OmlTranslateCommand.cpp)
add_subcommand(inf core sampling SamplingCommand.cpp)
add_subcommand(inf core tec-to-snap TecplotToSnapCommand.cpp)
add_subcommand(inf adaptation metric MetricCommand.cpp)
add_subcommand(inf adaptation refine-metric-to-snap RefineMetricToSnapCommand.cpp)
add_subcommand(inf adaptation adapt AdaptCommand.cpp)
add_subcommand(inf adaptation metric-stats MetricStatsCommand.cpp)
add_subcommand(inf experimental color MeshColorCommand.cpp)
add_subcommand(inf experimental wiggle WiggleCommand.cpp)
add_subcommand(inf experimental raytrace RaytracingCommand.cpp)
add_subcommand(inf experimental snap-diff SnapDiffCommand.cpp)
add_subcommand(inf experimental iextrude ExtrudeCommand.cpp)
add_subcommand(inf profiling line-sampling-profiler LineSamplingProfiling.cpp)

add_executable(nml nml.cpp)
target_compile_definitions(nml PRIVATE DRIVER_PREFIX="nml")
target_link_libraries(nml PUBLIC
        parfait::parfait
        infinity::infinity
        tracer::tracer
        )
set_standard_ring_rpath(nml)
ring_install_executable(TARGETS nml EXPORT infinity-targets DESTINATION bin)

add_subcommand(nml core patch PatchCommand.cpp)

if (${RING_INSTALL_DEV_TOOLS})
    add_subcommand(inf devtools analyze-trace AnalyzeTraceCommand.cpp)
    add_subcommand(inf devtools debug-tecplot TecplotReaderCommand.cpp)
    add_subcommand(inf devtools reorder ReorderingCommand.cpp)
    add_subcommand(inf devtools hessian ScalarToHessianCommand.cpp)
    add_subcommand(inf devtools iextrude ExtrudeCommand.cpp)
endif ()


set(UNIT_TEST_SRC
        AnalyzeTraceTests.cpp
        MapBcLumpingTests.cpp
        )
if (RING_BUILD_TESTING)
    add_catch_unit_test(subcommand_unit_tests ${UNIT_TEST_SRC})
    target_link_libraries(subcommand_unit_tests PUBLIC
            parfait::parfait
            infinity::infinity
            )
    add_subdirectory(cli_testing)
endif ()


install(FILES setup-infinity.tcsh DESTINATION python)


