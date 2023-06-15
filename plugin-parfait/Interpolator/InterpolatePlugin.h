#pragma once
#include <mpi.h>
#include <memory>
#include <t-infinity/InterpolationInterface.h>

class ParfaitInterpolationFactory : public inf::InterpolationFactory {
  public:
    virtual std::shared_ptr<inf::InterpolationInterface> create(std::shared_ptr<inf::MeshInterface> donor,
                                                                std::shared_ptr<inf::MeshInterface> receptor,
                                                                MPI_Comm comm) const override;
};

extern "C" {
std::shared_ptr<ParfaitInterpolationFactory> createInterpolationFactory();
}
