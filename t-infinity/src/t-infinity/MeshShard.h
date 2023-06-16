#pragma once
#include "PluginLoader.h"
#include "MeshInterface.h"

namespace inf {
class MeshShard {
  public:
    virtual std::shared_ptr<MeshInterface> shard(MPI_Comm comm,
                                                 const MeshInterface& mesh) const = 0;
    virtual ~MeshShard() = default;
};
inline auto getMeshShard(const std::string& directory, const std::string& name) {
    return PluginLoader<MeshShard>::loadPlugin(directory, name, "createMeshShard");
}
}
