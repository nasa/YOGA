# This is done to ensure this is run last in the cmake install order
if( ${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.21)
    set(extra_install_args ALL_COMPONENTS)
endif()

install(SCRIPT RingPostInstall.cmake ${extra_install_args})

if(NOT WIN32)
    string(ASCII 27 Esc)
    set(ColorReset "${Esc}[m")
    set(ColorBold  "${Esc}[1m")
    set(Red         "${Esc}[31m")
    set(Green       "${Esc}[32m")
endif()

string(REPLACE ";" "\n  " packages_with_newlines "${packages_to_build}")
install(CODE
        "message(\"\n${Green}${ColorBold}Successfully installed:\n  ${packages_with_newlines}${ColorReset}\")"
        ${extra_install_args})
