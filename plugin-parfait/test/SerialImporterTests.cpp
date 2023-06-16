#include <t-infinity/TinfMesh.h>
#include <RingAssertions.h>
#include "MockReaders.h"
#include "SerialMeshImporter.h"

TEST_CASE("Given a Reader, produce a Mesh in serial") {
    TwoTetReader reader;

    auto mesh = SerialMeshImporter::import(reader);

    REQUIRE(8 == mesh->nodeCount());
    REQUIRE(2 == mesh->cellCount(inf::MeshInterface::TETRA_4));
    for (int i = 0; i < mesh->nodeCount(); i++) {
        long gid = mesh->globalNodeId(i);
        REQUIRE(gid == i);
    }
}
