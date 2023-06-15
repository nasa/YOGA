#pragma once
#include <mpi.h>
#include <memory>
#include <string>
#include "FieldInterface.h"
#include "MeshInterface.h"
#include "PluginLoader.h"

namespace inf {
class MetricCalculatorInterface {
  public:
    virtual std::shared_ptr<FieldInterface> calculate(std::shared_ptr<MeshInterface> mesh,
                                                      MPI_Comm comm,
                                                      std::shared_ptr<FieldInterface> field,
                                                      std::string parameters) const = 0;
    virtual std::shared_ptr<FieldInterface> calculateFromMultiple(
        std::shared_ptr<MeshInterface> mesh,
        MPI_Comm comm,
        std::vector<std::shared_ptr<FieldInterface>> field,
        std::string parameters) const = 0;
    virtual ~MetricCalculatorInterface() = default;
};

inline std::shared_ptr<inf::MetricCalculatorInterface> getMetricCalculator(
    const std::string& path, const std::string& name) {
    return PluginLoader<MetricCalculatorInterface>::loadPlugin(
        path, name, "createMetricCalculator");
}
}
