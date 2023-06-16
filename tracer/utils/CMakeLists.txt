install(FILES analyze_trace.py
        RENAME analyze_trace
        DESTINATION bin
        PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE)
install(FILES merge.py
        RENAME merge_trace_files
        DESTINATION bin
        PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE)
install(DIRECTORY TracerUtils
        DESTINATION python)

if (RING_BUILD_TESTING)
    add_test(NAME analyze_trace_tests COMMAND "python" "analyze_trace_tests.py" WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    set_tests_properties(analyze_trace_tests PROPERTIES PROCESSORS 1)
    set_tests_properties(analyze_trace_tests PROPERTIES ENVIRONMENT PYTHONPATH=${CMAKE_INSTALL_PREFIX}/python)
endif ()
