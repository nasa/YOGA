#pragma once
#include <mpi.h>
#include <memory>
#include <string>
#include "PluginLoader.h"
#include "ByteStreamInterface.h"

namespace inf {
class ByteStreamLoaderInterface {
  public:
    virtual std::shared_ptr<ByteStreamInterface> load(std::string filename,
                                                      MPI_Comm comm) const = 0;
    virtual ~ByteStreamLoaderInterface() = default;
};
inline auto getByteStreamLoader(const std::string& directory, const std::string& name) {
    return PluginLoader<ByteStreamLoaderInterface>::loadPlugin(
        directory, name, "createByteStreamLoader");
}
}
