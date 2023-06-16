#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/MeshConnectivity.h>

TEST_CASE("Can create basic") {
    auto mesh = inf::CartMesh::create(2, 2, 2);
    auto c2c = inf::CellToCell::build(*mesh);
    REQUIRE(int(c2c.size()) == mesh->cellCount());
    for (auto& neighbors : c2c) {
        REQUIRE(!neighbors.empty());
    }
}

TEST_CASE("Can create if there are bars and nodes") {
    auto mesh = inf::CartMesh::createWithBarsAndNodes(2, 2, 2);
    auto c2c = inf::CellToCell::build(*mesh);
    REQUIRE(int(c2c.size()) == mesh->cellCount());
    for (auto& neighbors : c2c) {
        REQUIRE(!neighbors.empty());
    }
}

TEST_CASE("Can only get the elements of certain dimension") {
    auto mesh = inf::CartMesh::createWithBarsAndNodes(2, 2, 2);
    for (int dimension : {3, 2, 1}) {
        INFO("Dimension: " << dimension);
        auto c2c = inf::CellToCell::buildDimensionOnly(*mesh, dimension);
        REQUIRE(int(c2c.size()) == mesh->cellCount());
        for (int c = 0; c < int(c2c.size()); c++) {
            if (mesh->cellDimensionality(c) == dimension) {
                REQUIRE_FALSE(c2c[c].empty());
            } else {
                REQUIRE(c2c[c].empty());
            }
        }
    }
}
