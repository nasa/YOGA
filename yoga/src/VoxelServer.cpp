#include <Tracer.h>
#include "VoxelServer.h"
#include <parfait/ExtentBuilder.h>
#include "LoadBalancer.h"

namespace YOGA {
VoxelServer::VoxelServer(LoadBalancer& L) : loadBalancer(L) {
    printf("Yoga: load balancer starting with %i work units\n", loadBalancer.getRemainingVoxelCount());
}
MessagePasser::Message VoxelServer::doWork(MessagePasser::Message& msg) {
    MessagePasser::Message reply;
    if (loadBalancer.getRemainingVoxelCount() % 50 == 0 and loadBalancer.getRemainingVoxelCount() > 0)
        printf("     load balancer: %i units remaining\n", loadBalancer.getRemainingVoxelCount());
    Tracer::begin("Get next voxel");
    reply.pack(loadBalancer.getRemainingVoxelCount());
    reply.pack(getNextVoxel());
    Tracer::end("Get next voxel");
    return reply;
}

Parfait::Extent<double> VoxelServer::getNextVoxel() {
    return 0 < loadBalancer.getRemainingVoxelCount() ? loadBalancer.getWorkVoxel()
                                                     : Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
}
}
