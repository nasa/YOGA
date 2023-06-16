#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mpi.h>
#include "FieldInterface.h"
#include "MeshInterface.h"
#include "PluginLoader.h"

namespace inf {

class FieldLoader {
  public:
    virtual ~FieldLoader() = default;
    virtual void load(const std::string& filename,
                      MPI_Comm comm,
                      const inf::MeshInterface& mesh) = 0;
    virtual std::vector<std::string> availableFields() const = 0;
    virtual std::shared_ptr<inf::FieldInterface> retrieve(const std::string&) const = 0;
};

inline auto getFieldLoader(const std::string& directory, const std::string& name) {
    return PluginLoader<FieldLoader>::loadPlugin(directory, name, "createFieldLoader");
}
}