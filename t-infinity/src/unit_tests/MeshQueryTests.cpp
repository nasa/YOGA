#include <RingAssertions.h>
#include "MockMeshes.h"
#include <t-infinity/MeshInquisitor.h>

using namespace inf;


TEST_CASE("extract tet"){
    auto mesh = mock::getSingleTetMesh();
    MeshInquisitor inquisitor(*mesh);

    auto cell = inquisitor.cell(4);
    REQUIRE(4 == cell.size());
    cell = inquisitor.cell(0);
    REQUIRE(3 == cell.size());

    auto vertices = inquisitor.vertices(4);
    REQUIRE(4 == vertices.size());

    double volume = inquisitor.cellVolume(4);
    REQUIRE(volume > 0.0);

    double area = inquisitor.cellArea(0);
    REQUIRE(area > 0.0);
}

TEST_CASE("extract prism"){
    auto mesh = mock::getSingleHexMesh();
    MeshInquisitor inquisitor(*mesh);

    auto nodes = inquisitor.cell(0);
    REQUIRE(8 == nodes.size());

    double volume = inquisitor.cellVolume(0);
    REQUIRE(volume > 0.0);
}
