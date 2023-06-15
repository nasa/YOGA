function(set_one_ring_kokkos_defaults)
    if(RING_ENABLE_CUDA)
        set(Kokkos_ENABLE_CUDA
                ON
                CACHE BOOL "Enable NVIDIA GPU support")
        set(Kokkos_ENABLE_CUDA_LAMBDA
                ON
                CACHE BOOL "Enable using lambdas with CUDA")
        set(Kokkos_ENABLE_CUDA_CONSTEXPR
                ON
                CACHE BOOL "Enable calling STL constexpr functions")
        set(Kokkos_ENABLE_CUDA_LDG_INTRINSIC
                ON
                CACHE BOOL "Enables texture memory fetchs on NVIDIA GPUs")
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STRICT_ANSI__")
        endif()
    endif()
    if(CMAKE_BUILD_TYPE MATCHES Debug)
      set(Kokkos_ENABLE_DEBUG
              ON
              CACHE BOOL "Enable Kokkos debug support")
      set(Kokkos_ENABLE_DEBUG_BOUNDS_CHECK
              ON
              CACHE BOOL "Enable Kokkos bounds checking")
    endif()
    set(Kokkos_ENABLE_DEPRECATED_CODE_3
            OFF
            CACHE BOOL "Explicitly disable Kokkos deprecated code")
    set(Kokkos_ENABLE_SERIAL
            ON
            CACHE BOOL "Explicitly enable Kokkos SERIAL backend by default")
endfunction()
