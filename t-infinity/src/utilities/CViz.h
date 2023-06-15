#pragma once
#include <string>
#include <t-infinity/Mangle.h>

extern "C" {
void mangle(visualize)(int fortran_comm, const char* filename, void* tinf_mesh, void* solution);
void mangle(visualize_with_options)(
    int fortran_comm, const char* filename, void* tinf_mesh, void* solution, const char* options);
void mangle(scatter_plot)(int fortran_comm,
                  const char* filename,
                  int npts,
                  void (*get_xyz)(int, double*),
                  void (*get_gid)(int, long*),
                  void (*get_scalar)(int, double*));
}