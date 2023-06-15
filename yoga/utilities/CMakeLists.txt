function(add_yogacommand extension_name command_name SOURCE_FILES)
    add_subcommand(yoga ${extension_name} ${command_name} ${SOURCE_FILES})
    subcommand_link_libraries(yoga ${extension_name} ${command_name} PRIVATE yoga YogaPlugin)
endfunction()

if (CMAKE_CXX_COMPILER MATCHES nvcc_wrapper)
    # To avoid lots of nvcc warnings about std::complex being used in CUDA kernels 
    set_source_files_properties(AssembleCommand.cpp PROPERTIES COMPILE_OPTIONS --host-only)
endif()

add_subcommand(inf experimental cube-sampling CubeSamplingCommand.cpp CubeSampling.h)
subcommand_link_libraries(inf experimental cube-sampling yoga)

add_yogacommand(core extensions ExtensionsCommand.cpp)
add_yogacommand(core composite-rotor CompositeRotorCommand.cpp)
add_yogacommand(core make-composite MakeCompositeCommand.cpp)
add_yogacommand(core assemble AssembleCommand.cpp)
add_yogacommand(core show-syntax InputSyntaxCommand.cpp)
add_yogacommand(core verify VerifyNodeStatusCommand.cpp)
add_yogacommand(core split-csv CompositeCsvSplitterCommand.cpp)
add_yogacommand(core check-dcif CheckDcifCommand.cpp)
add_yogacommand(core plot-dcif PlotDciCommand.cpp)
add_yogacommand(core check-syntax CheckSyntaxCommand.cpp)
add_yogacommand(experimental fix-orphan FixOrphanCommand.cpp)
add_yogacommand(experimental rotate-metric RotateMetricCommand.cpp)

add_executable(yoga_exe yoga.cpp)
set_target_properties(yoga_exe PROPERTIES OUTPUT_NAME yoga)
target_compile_definitions(yoga_exe PRIVATE DRIVER_PREFIX="yoga")
target_link_libraries(yoga_exe PUBLIC
        parfait::parfait
        infinity::infinity
        tracer::tracer
        yoga
        )
set_standard_ring_rpath(yoga_exe)

install(TARGETS yoga_exe EXPORT yoga-targets DESTINATION bin)

add_subdirectory(tests)