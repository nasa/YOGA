function(add_example CASE)
    set(TEST_NAME t-infinity-example-${CASE})
    add_test(NAME ${TEST_NAME} COMMAND "./run_example.sh" WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${CASE})
    # FIXME: openmpi hangs when launching in parallel
    set_tests_properties(${TEST_NAME} PROPERTIES RUN_SERIAL TRUE)
endfunction()

add_example(1-cell)
add_example(mesh-adaptation)
add_example(overset)
add_example(sampling)
