#include <t-infinity/AddMissingFaces.h>
#include <t-infinity/BoundaryNodes.h>
#include <t-infinity/CartMesh.h>

#include <RingAssertions.h>

TEST_CASE("Boundary nodes") {
    std::shared_ptr<inf::MeshInterface> mesh = inf::CartMesh::create(1, 1, 1);
    auto boundary_nodes = inf::BoundaryNodes::buildSet(*mesh);
    REQUIRE(boundary_nodes.size() == 8);
}

TEST_CASE("Interior nodes") {
    std::shared_ptr<inf::MeshInterface> mesh = inf::CartMesh::create(1, 1, 1);
    auto interior_nodes = inf::InteriorNodes::buildSet(*mesh);
    REQUIRE(interior_nodes.size() == 0);
}
