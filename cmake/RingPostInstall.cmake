function(move_ref_install_file NAME)
    if(EXISTS ${CMAKE_INSTALL_PREFIX}/bin/${NAME})
        file(RENAME ${CMAKE_INSTALL_PREFIX}/bin/${NAME} ${CMAKE_INSTALL_PREFIX}/bin/inf_${NAME})
    endif()
endfunction()

move_ref_install_file(ref)
move_ref_install_file(refmpi)
move_ref_install_file(refmpifull)
