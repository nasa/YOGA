#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include "t-infinity/MeshMemoryConsumption.h"

TEST_CASE("Can estimate the size of a mesh in memory") {
    auto mesh = inf::CartMesh::create(10, 10, 10);

    auto mesh_memory_consumption = inf::MeshMemoryConsumption::calc(*mesh);
    size_t xyz_size = mesh->nodeCount() * 3 * sizeof(double);
    REQUIRE(mesh_memory_consumption.xyz_size_bytes == xyz_size);

    size_t c2n_size_bytes = 0;
    for (int c = 0; c < mesh->cellCount(); c++) {
        auto length = mesh->cellLength(c);
        c2n_size_bytes += length * sizeof(int);
    }
    REQUIRE(c2n_size_bytes == mesh_memory_consumption.c2n_size_bytes);

    REQUIRE(mesh->cellCount() * sizeof(int) == mesh_memory_consumption.tag_size_bytes);

    REQUIRE(mesh_memory_consumption.ghost_total == 0);

    REQUIRE(mesh_memory_consumption.total == 115116);
}