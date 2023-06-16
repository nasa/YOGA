#include <RingAssertions.h>
#include <../shared/SU2IO.h>
#include "t-infinity/CartMesh.h"

TEST_CASE("Can read 2D SU2 files") {
    auto filename = std::string(GRIDS_DIR) + "/su2/example.su2";

    auto reader = SU2Reader(filename);
    REQUIRE(reader.dimension == 2);
    REQUIRE(reader.nodeCount() == 36);
    REQUIRE(reader.points.size() == 36);
    REQUIRE(reader.cellCount(inf::MeshInterface::TRI_3) == 54);
    REQUIRE(reader.tag_count == 4);
}
TEST_CASE("Can read a more tortured real-life su2 file") {
    auto filename = std::string(GRIDS_DIR) + "/su2/flatplate_137x097.su2";

    auto reader = SU2Reader(filename);
    REQUIRE(reader.dimension == 2);
    REQUIRE(reader.nodeCount() == 13289);
    REQUIRE(reader.points.size() == 13289);
    REQUIRE(reader.cellCount(inf::MeshInterface::QUAD_4) == 13056);
    REQUIRE(reader.tag_count == 5);
    REQUIRE(reader.tag_names[0] == "farfield");
    REQUIRE(reader.tag_names[4] == "wall");
}

TEST_CASE("Can write su2 files") {
    auto mesh = inf::CartMesh::create(5, 5, 5);
    SU2Writer writer("dog.su2", mesh);
    writer.write();
}