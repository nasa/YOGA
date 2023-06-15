
if(RING_BUILD_TESTING)
    add_subdirectory(unit_tests)
    add_subdirectory(tests)
endif()

add_subdirectory(t-infinity-interfaces)
add_subdirectory(t-infinity)

if(BUILD_PYTHON_RUNTIME)
    add_subdirectory(t-py)
endif()
add_subdirectory(utilities)


install(FILES t-infinityConfig.cmake DESTINATION cmake)
