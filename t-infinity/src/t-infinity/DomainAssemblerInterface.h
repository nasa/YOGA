#pragma once
#include <mpi.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "FieldInterface.h"
#include "MeshInterface.h"
#include "PluginLoader.h"

namespace inf {
class DomainAssemblerInterface {
  public:
    virtual std::vector<int> performAssembly() = 0;
    virtual std::vector<int> determineGridIdsForNodes() = 0;
    virtual std::map<int, std::vector<double>> updateReceptorSolutions(
        std::function<void(int, double, double, double, double*)> getter) const = 0;
    virtual std::vector<std::string> listFields() const = 0;
    virtual std::shared_ptr<FieldInterface> field(std::string field_name) const = 0;
    virtual ~DomainAssemblerInterface() = default;
};

class DomainAssemblerFactory {
  public:
    virtual std::shared_ptr<DomainAssemblerInterface> createDomainAssembler(
        MPI_Comm comm, const MeshInterface& mesh, int component_id, std::string json) const = 0;
    virtual ~DomainAssemblerFactory() = default;
};

inline auto getDomainAssemblerFactory(const std::string& directory, const std::string& name) {
    return PluginLoader<DomainAssemblerFactory>::loadPlugin(
        directory, name, "createDomainAssemblerFactory");
}

}
