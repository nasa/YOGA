#pragma once
#include <t-infinity/Mangle.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/MeshLoader.h>

extern "C" {
int32_t mangle(tinf_preprocessor_create)(void** mesh, void* problem, void* comm);
int32_t mangle(tinf_preprocessor_destroy)(void** mesh);
}

int32_t mangle(tinf_preprocessor_create)(void** mesh, void* problem, void* comm) {
    // get path to t-inf runtime and hope its the same place this plugin lives
    auto path = inf::getPluginDir();
    auto preprocessor = inf::getMeshLoader(path, "NC_PreProcessor");

    // magic filename from imaginary "problem"
    auto filename = (const char*)problem;
    printf("Loading grid: %s\n", filename);

    // magic a real communicator from what the what is stored in comm
    auto fortran_comm_ptr = (int*)comm;
    int fortran_comm = *fortran_comm_ptr;
    MPI_Comm real_mpi_comm = MPI_Comm_f2c(fortran_comm);

    auto shared_mesh = preprocessor->load(real_mpi_comm, filename);

    *mesh = new SharedPointerWrapper<inf::MeshInterface>(shared_mesh);

    return TINF_SUCCESS;
}

int32_t mangle(tinf_preprocessor_destroy)(void** mesh) {
    auto ptr = (SharedPointerWrapper<inf::MeshInterface>*)(*mesh);
    delete ptr;
    return TINF_SUCCESS;
}
