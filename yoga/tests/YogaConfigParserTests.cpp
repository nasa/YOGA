#include <RingAssertions.h>
#include "YogaConfiguration.h"

using namespace YOGA;


TEST_CASE("control which ranks to trace"){
    std::string s = "trace 0 193 24 ";

    YogaConfiguration config(s);

    auto trace_ranks = config.getRanksToTrace();
    REQUIRE(3 == trace_ranks.size());
    REQUIRE(1 == trace_ranks.count(0));
    REQUIRE(1 == trace_ranks.count(193));
    REQUIRE(1 == trace_ranks.count(24));
}

TEST_CASE("get extra layer count"){
    std::string s = "extra-layers-for-interpolation-bcs 2";

    YogaConfiguration config(s);

    int n = config.numberOfExtraLayersForInterpBcs();
    REQUIRE(2 == n);
}

TEST_CASE("set rcb agglomeration"){
    std::string s = "rcb 128";
    YogaConfiguration config(s);
    int n = config.rcbAgglomerationSize();
    REQUIRE(128 == n);
}

TEST_CASE("dump fun3d-part-file"){
    std::string s = "dump fun3d-part-file";
    YogaConfiguration config(s);
    REQUIRE(config.shouldDumpPartFile());
}

TEST_CASE("dump partition-aabb"){
    std::string s = "dump partition-extents";
    YogaConfiguration config(s);
    REQUIRE(config.shouldDumpPartitionExtents());
}

TEST_CASE("set component grid importance"){
    std::string s = "component-grid-importance 0 0 0 3 5";
    YogaConfiguration config(s);
    auto importance = config.getComponentGridImportance();
    REQUIRE(5 == importance.size());
    std::vector<int> expected {0,0,0,3,5};
    REQUIRE(expected == importance);
}

TEST_CASE("the whole enchilada"){
    std::string s = "extra-layers-for-interpolation-bcs 2 "
                    "target-voxel-size 20000 "
                    "max-receptors "
                    "load-balancer 1";

    YogaConfiguration config(s);

    int n = config.numberOfExtraLayersForInterpBcs();
    REQUIRE(2 == n);

    REQUIRE(20000 == config.selectedTargetVoxelSize());
    REQUIRE(config.shouldAddExtraReceptors());
    REQUIRE(1 == config.selectedLoadBalancer());
}

