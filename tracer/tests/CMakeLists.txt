add_executable(tracer_smoke_test main.cpp ../include/Tracer.h)
target_link_libraries(tracer_smoke_test tracer)


add_executable(performance_test PerformanceTest.cpp ../include/Tracer.h)
target_link_libraries(performance_test tracer)

add_executable(tracer_main_cxx main.cpp)
target_link_libraries(tracer_main_cxx
        tracer
        )

add_executable(tracer_main_c main.c)
target_link_libraries(tracer_main_c
        tracer
        )
set_target_properties(tracer_main_c PROPERTIES LINKER_LANGUAGE CXX)

option(tracer_ENABLE_FORTRAN "Build tracer with fortran wrapper" OFF)
include(CheckLanguage)
if (tracer_ENABLE_FORTRAN)
    check_language(Fortran)
    # If we have a valid fortran compiler, build tracer Fortran utility
    if (CMAKE_Fortran_COMPILER)
        enable_language(Fortran)
        add_executable(tracer_main_f90 main.F90 tracing_wrapper.F90)
        set_target_properties(tracer_main_f90 PROPERTIES LINKER_LANGUAGE Fortran)
        target_link_libraries(tracer_main_f90 tracer)
    else()
        message(FATAL_ERROR "tracer_ENABLE_FORTRAN=ON but a valid Fortran compiler was not detected.")
    endif()
endif()

enable_testing()
add_test(NAME tracer COMMAND "./tracer_smoke_test")

add_catch_unit_test(TracerTests TracerTests.cpp)
target_link_libraries(TracerTests PUBLIC tracer::tracer)
