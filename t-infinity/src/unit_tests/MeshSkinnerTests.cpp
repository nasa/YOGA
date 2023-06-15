#include <RingAssertions.h>
#include "MockMeshes.h"
#include <t-infinity/AddMissingFaces.h>

using namespace inf;
TEST_CASE("Add missing surface elements to a mesh") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = std::make_shared<TinfMesh>(mock::twoTouchingTets(), 0);
    auto other_mesh = addMissingFaces(mp, *mesh);

    REQUIRE(8 == other_mesh->cellCount());
}
