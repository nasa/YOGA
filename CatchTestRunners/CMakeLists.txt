cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(CatchTestRunners)

set(CMAKE_CXX_STANDARD 14)

# NOTE: This is purely to be used in one-ring to speedup compilation of catch unit tests.
#       This package should not be relied upon by external projects.

find_package(Catch2 3 REQUIRED)
find_package(MPI QUIET)
find_package(Kokkos QUIET)

set(TestRunnerSupport Catch2)
add_library(Runner OBJECT CatchUnitTestRunner.cpp)
add_library(CatchTestRunners::Runner ALIAS Runner)
target_include_directories(Runner PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )
target_link_libraries(Runner PUBLIC Catch2::Catch2)

if (NOT RING_DISABLE_KOKKOS AND Kokkos_FOUND)
    set(TestRunnerSupport ${TestRunnerSupport} Kokkos)
    target_link_libraries(Runner PRIVATE Kokkos::kokkos)
    target_compile_definitions(Runner PRIVATE HAVE_KOKKOS)
    ring_precompiled_headers(Runner PRIVATE <Kokkos_Core.hpp>)
    if(Kokkos_ENABLE_OPENMP)
        find_package(OpenMP)
        target_link_libraries(Runner PRIVATE OpenMP::OpenMP_CXX)
    endif()
endif()

if (MPI_FOUND)
    set(TestRunnerSupport ${TestRunnerSupport} MPI)
    target_link_libraries(Runner PUBLIC ${MPI_LIBRARIES})
    target_include_directories(Runner PRIVATE ${MPI_INCLUDE_PATH})
    target_compile_definitions(Runner PRIVATE HAVE_MPI)
endif()

set_standard_ring_rpath(Runner)

message("building with support for: ${TestRunnerSupport}")