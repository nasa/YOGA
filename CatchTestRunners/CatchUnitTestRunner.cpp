#ifdef HAVE_MPI
#include <mpi.h>
#endif

#if HAVE_KOKKOS
#include <Kokkos_Core.hpp>
#endif

#include <RingAssertions.h>

int main(int argc, char* argv[]) {
#if HAVE_MPI
    MPI_Init(NULL, NULL);
#endif

#if HAVE_KOKKOS
    Kokkos::InitializationSettings arguments;
#if defined(__CUDACC__)
    // Only test on single GPU, to avoid accidentally
    // getting the display graphics card mixed with the HPC GPU.
    arguments.set_num_devices(1);
#else
    arguments.set_disable_warnings(true);
#endif
    Kokkos::initialize(arguments);
#endif

    int result = Catch::Session().run(argc, argv);

#if HAVE_KOKKOS
    Kokkos::finalize();
#endif

#if HAVE_MPI
    MPI_Finalize();
#endif

    return (result < 0xff ? result : 0xff);
}
