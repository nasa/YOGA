
set(RING_CXX_STANDARD 14)
set(RING_IS_CXX14_SUPPORTED "YES")
set(RING_IS_CXX17_SUPPORTED "NO")
set(RING_IS_CXX20_SUPPORTED "NO")

# We could check up to 20, but it looks like Intel 2019 says it has C++20 support
# But then fails to allow the [nodiscard] attribute
# Since we're not really looking to use C++20 at the moment we just use up to C++17
set(RING_MAX_CXX_TO_CHECK ${RING_MAX_ATTEMPTED_CXX})

message("Detecting C++ standard compiler support")
message("Starting with C++14:  Maximum to detect: C++${RING_MAX_CXX_TO_CHECK}")
message("  Compiler: ${CMAKE_CXX_COMPILER}")
message("  Version:  ${CMAKE_CXX_COMPILER_VERSION}")


if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "7")
        message("Compiler detected: GCC ${CMAKE_CXX_COMPILER_VERSION}")
        message("Even though this compiler claims to support C++17 it incorrectly performs *this capture")
        message("Lowering C++ version to 14")
        set(RING_MAX_CXX_TO_CHECK 14)
    endif ()
endif ()

if ("${RING_MAX_CXX_TO_CHECK}" GREATER_EQUAL 17)
    if ("${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_17")
        set(RING_CXX_STANDARD 17)
        set(RING_IS_CXX17_SUPPORTED "YES")
    else()
        set(RING_MAX_CXX_TO_CHECK 14)
    endif ()
endif ()

if ("${RING_MAX_CXX_TO_CHECK}" GREATER_EQUAL 20)
    if ("${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_20")
        set(RING_CXX_STANDARD 20)
        set(RING_IS_CXX20_SUPPORTED "YES")
    else()
        set(RING_MAX_CXX_TO_CHECK 17)
    endif ()
endif ()

set(CMAKE_CXX_STANDARD ${RING_CXX_STANDARD})
message("Settings one-ring CXX standard to ${RING_CXX_STANDARD}")
