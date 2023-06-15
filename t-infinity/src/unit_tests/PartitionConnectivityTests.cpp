#include <RingAssertions.h>
#include "MockMeshes.h"
#include <t-infinity/PartitionConnectivityMapper.h>

using namespace inf;

TEST_CASE("given a mesh, make a graph of connectivity between partitions") {
    MessagePasser mp(MPI_COMM_WORLD);

    if (mp.NumberOfProcesses() == 1) {
        auto mesh = inf::mock::TwoTetMesh();
        auto connectivity = PartitionConnectivityMapper::buildPartToPart(mesh, mp);
        REQUIRE(1 == connectivity.size());
        REQUIRE(0 == connectivity[0][0]);
    }
}

TEST_CASE("map to new owning cpu") {
    std::vector<int> owning_cpu_id{0, 1, 1, 0, 1, 1, 0, 0};

    auto partition_ids_per_cpu = PartitionConnectivityMapper::buildCPUToParts(owning_cpu_id);
    std::vector<std::vector<int>> expected_parts_per_cpu(2);
    expected_parts_per_cpu[0] = {0, 3, 6, 7};
    expected_parts_per_cpu[1] = {1, 2, 4, 5};

    REQUIRE(expected_parts_per_cpu == partition_ids_per_cpu);

    auto old_to_new = PartitionConnectivityMapper::buildOldPartToNew(owning_cpu_id);
    REQUIRE(8 == old_to_new.size());

    std::vector<int> expected_old_to_new(owning_cpu_id.size());
    expected_old_to_new[0] = 0;
    expected_old_to_new[3] = 1;
    expected_old_to_new[6] = 2;
    expected_old_to_new[7] = 3;

    expected_old_to_new[1] = 4;
    expected_old_to_new[2] = 5;
    expected_old_to_new[4] = 6;
    expected_old_to_new[5] = 7;

    REQUIRE(expected_old_to_new == old_to_new);
}
