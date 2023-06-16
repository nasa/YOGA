#include "InterpolatePlugin.h"
#include <t-infinity/PointCloudInterpolator.h>
#include <t-infinity/MeshInterface.h>

using namespace inf;

std::shared_ptr<inf::InterpolationInterface> ParfaitInterpolationFactory::create(
    std::shared_ptr<inf::MeshInterface> donor, std::shared_ptr<inf::MeshInterface> receptor, MPI_Comm comm) const {
    // for a true point cloud
    if (donor->cellCount()==0) return std::make_shared<FromPointCloudInterpolator>(comm, donor, receptor);
    // for cell based meshes
    return std::make_shared<Interpolator>(comm, donor, receptor);
}
std::shared_ptr<ParfaitInterpolationFactory> createInterpolationFactory() {
    return std::make_shared<ParfaitInterpolationFactory>();
}
