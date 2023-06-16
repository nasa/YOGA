#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/SurfaceMesh.h>

TEST_CASE("Trimesh can be created from quad mesh") {
    auto mesh = inf::CartMesh::createSurface(4, 4, 4);
    auto tri_mesh = inf::SurfaceMesh::triangulate(*mesh);
    REQUIRE(tri_mesh->nodeCount() == mesh->nodeCount());
}