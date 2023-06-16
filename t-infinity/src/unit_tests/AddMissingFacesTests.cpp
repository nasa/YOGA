#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/AddMissingFaces.h>
#include "MockMeshes.h"
#include <t-infinity/MeshHelpers.h>

using namespace inf;

TEST_CASE("Can add missing elements") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    INFO("Rank: " << mp.Rank());
    std::shared_ptr<MeshInterface> mesh = CartMesh::createVolume(1, 1, 1);
    REQUIRE(1 == mesh->cellCount());
    REQUIRE(mesh->cellCount(MeshInterface::HEXA_8) == mesh->cellCount());
    REQUIRE(mp.NumberOfProcesses() == globalCellCount(mp, *mesh));
    REQUIRE(mp.NumberOfProcesses() == globalCellCount(mp, *mesh, MeshInterface::HEXA_8));
    mesh = addMissingFaces(mp, *mesh);
    REQUIRE(7 == mesh->cellCount());
    REQUIRE(1 == mesh->cellCount(MeshInterface::HEXA_8));
    REQUIRE(6 == mesh->cellCount(MeshInterface::QUAD_4));
    std::set<long> gids;
    for (int cell_id = 0; cell_id < mesh->cellCount(); ++cell_id) {
        gids.insert(mesh->globalCellId(cell_id));
    }

    REQUIRE(std::set<long>{0,1,2,3,4,5,6} == gids);
}
