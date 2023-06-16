#include <RingAssertions.h>
#include "Connectivity.h"

using namespace YOGA;

auto generateConnectivityMockMesh() {
    YogaMesh mesh;
    mesh.setNodeCount(5);
    mesh.setCellCount(2);
    mesh.setCells([](int) { return 4; },
                  [](int id, int* cell) {
                      for (int i = 0; i < 3; i++) cell[i] = i;
                      cell[3] = 0 == id ? 3 : 4;
                  });
    return mesh;
}

TEST_CASE("build node to cell connectivity") {
    auto mesh = generateConnectivityMockMesh();
    auto n2c = Connectivity::nodeToCell(mesh);

    REQUIRE(5 == n2c.size());
    REQUIRE(2 == n2c[0].size());
    REQUIRE(2 == n2c[1].size());
    REQUIRE(2 == n2c[2].size());
    REQUIRE(1 == n2c[3].size());
    REQUIRE(1 == n2c[4].size());
}

TEST_CASE("build Yoga nbr connectivity") {
    auto mesh = generateConnectivityMockMesh();
    auto n2n = Connectivity::nodeToNode(mesh);

    REQUIRE(5 == n2n.size());
    REQUIRE(4 == n2n[0].size());
    REQUIRE(4 == n2n[1].size());
    REQUIRE(4 == n2n[2].size());
    REQUIRE(3 == n2n[3].size());
    REQUIRE(3 == n2n[4].size());
}