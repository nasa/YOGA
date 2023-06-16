#include <RingAssertions.h>
#include "MockMeshes.h"
#include <MessagePasser/MessagePasser.h>
#include <parfait/ToString.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/SurfaceEdgeNeighbors.h>
#include <t-infinity/SurfaceMesh.h>

auto boxSurfaceMesh(MessagePasser mp, int nx, int ny, int nz) {
    auto mesh = inf::CartMesh::create(nx, ny, nz);
    auto surface_filter = inf::FilterFactory::surfaceFilter(mp.getCommunicator(), mesh);
    return surface_filter->getMesh();
}

TEST_CASE("Identifies manifold mesh") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = boxSurfaceMesh(mp, 2, 2, 2);
    REQUIRE(mesh->cellCount() == 4 * 6);
    auto surface_edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(*mesh);
    REQUIRE(surface_edge_neighbors.size() == 24);
    REQUIRE(inf::SurfaceMesh::isManifold(*mesh, surface_edge_neighbors));
}

TEST_CASE("Identifies non-manifold mesh") {
    auto mesh = std::make_shared<inf::TinfMesh>(inf::mock::oneTriangle(), 0);
    auto surface_edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(*mesh);
    REQUIRE(not inf::SurfaceMesh::isManifold(*mesh, surface_edge_neighbors));
}

TEST_CASE("Orientated mesh quads"){
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = boxSurfaceMesh(mp, 2, 2, 2);
    REQUIRE(mesh->cellCount() == 4 * 6);
    auto surface_edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(*mesh);
    REQUIRE(inf::SurfaceMesh::isOriented(*mesh, surface_edge_neighbors));
}
TEST_CASE("Orientated mesh tris"){
    auto mesh = inf::mock::threeManifoldTriangles();
    REQUIRE(mesh->cellCount() == 4);
    auto surface_edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(*mesh);
    REQUIRE(inf::SurfaceMesh::isOriented(*mesh, surface_edge_neighbors));
}
TEST_CASE("Non oriented trimesh"){
    auto mesh = inf::mock::threeManifoldTriangles();
    REQUIRE(mesh->cellCount() == 4);
    // -- reverse the first triangle
    mesh->mesh.cells[inf::MeshInterface::TRI_3][0] = 0;
    mesh->mesh.cells[inf::MeshInterface::TRI_3][1] = 1;
    mesh->mesh.cells[inf::MeshInterface::TRI_3][2] = 2;
    auto surface_edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(*mesh);
    REQUIRE_FALSE(inf::SurfaceMesh::isOriented(*mesh, surface_edge_neighbors));
}
