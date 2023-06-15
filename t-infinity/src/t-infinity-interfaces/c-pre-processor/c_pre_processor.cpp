#include "c_pre_processor.h"
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/MeshLoader.h>
#include <t-infinity/Mangle.h>
#include <t-infinity/MeshSanityChecker.h>

int32_t
tinf_preprocess(void **tinf_mesh, void *mpi_comm, const char *name)
{
  MPI_Comm mp_comm = *(MPI_Comm *)mpi_comm;
  std::string filename = name;
  try
  {
    auto pre_processor =
        inf::getMeshLoader(inf::getPluginDir(), "NC_PreProcessor");
    auto mesh = pre_processor->load(mp_comm, filename);
    // inf::MeshSanityChecker::checkAll(*mesh,mp_comm);
    *tinf_mesh = static_cast<void *>(
        new std::shared_ptr<inf::MeshInterface>(std::move(mesh)));
    return TINF_SUCCESS;
  }
  catch (std::exception &e)
  {
    printf("Caught unexpected exception: %s\n", e.what());
    return TINF_FAILURE;
  }
}
int32_t
tinf_mesh_destroy(void **tinf_mesh)
{
  std::shared_ptr<inf::MeshInterface> *casted_ptr =
      static_cast<std::shared_ptr<inf::MeshInterface> *>(*tinf_mesh);
  casted_ptr->reset();
  return TINF_SUCCESS;
}
