#include <RingAssertions.h>
#include <DistributedLoadBalancer.h>
#include <queue>
using namespace YOGA;

class MockLoadBalancer : public LoadBalancer{
  public:
    MockLoadBalancer(int n){
        for(int i=0;i<n;i++)
            voxels.push(Parfait::Extent<double>());
    }
    int getRemainingVoxelCount() {
        return voxels.size();
    }
    Parfait::Extent<double> getWorkVoxel() {
        auto e = voxels.front();
        voxels.pop();
        return e;
    }
  private:
    std::queue<Parfait::Extent<double>> voxels;
};

TEST_CASE("Split load balancer in 2, and put on different ranks"){
    MessagePasser mp(MPI_COMM_WORLD);
    MockLoadBalancer mock_balancer(5);
    DistributedLoadBalancer load_balancer(mp,mock_balancer,{0});
    if(0 == mp.Rank()) {
        REQUIRE(5 == load_balancer.getRemainingVoxelCount());
    }
}
