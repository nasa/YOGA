#include <RingAssertions.h>
#include <parfait/STLFactory.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/SurfaceEdgeNeighbors.h>
#include <t-infinity/STLConverters.h>
#include <t-infinity/Extract.h>
#include <t-infinity/SurfaceMesh.h>


TEST_CASE("can create edge neighbor tests for 2d elements similar to face neighbors for 3D") {
    auto facets = Parfait::STLFactory::getUnitCube();
    auto mesh = inf::STLConverters::stitchSTLToInfinity(facets);
    REQUIRE(mesh->nodeCount() == 8);
    REQUIRE(mesh->cellCount() == 12);

    auto edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(*mesh);
    REQUIRE(edge_neighbors.size() == 12);
    for(auto& neighbors : edge_neighbors){
        REQUIRE(neighbors.size() == 3);
        for(auto n : neighbors){
            REQUIRE((n >= 0 and n < 12));
        }
    }
}

TEST_CASE("Can fill multiple triangular shaped holes"){

    auto facets = Parfait::STLFactory::getUnitCube();
    facets.resize(facets.size()-1);
    facets.erase(facets.begin());
    auto mesh = inf::STLConverters::stitchSTLToInfinity(facets);
    REQUIRE(mesh->nodeCount() == 8);
    REQUIRE(mesh->cellCount() == 10);

    auto edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(*mesh);

    std::vector<std::array<int, 2>> non_manifold_edges = inf::SurfaceMesh::selectExposedEdges(*mesh, edge_neighbors);
    REQUIRE(non_manifold_edges.size() == 6);

    std::vector<std::vector<int>> loops = inf::SurfaceMesh::identifyLoops(non_manifold_edges);
    REQUIRE(loops.size() == 2);
    REQUIRE(loops[0].size() == 3);
    REQUIRE(loops[1].size() == 3);

    mesh = inf::SurfaceMesh::fillHoles(*mesh, loops);
    REQUIRE(mesh->cellCount() == 12);
}


TEST_CASE("Can fill a quad shaped holes"){

    auto facets = Parfait::STLFactory::getUnitCube();
    facets.resize(facets.size()-1);
    facets.resize(facets.size()-1);
    auto mesh = inf::STLConverters::stitchSTLToInfinity(facets);
    REQUIRE(mesh->nodeCount() == 8);
    REQUIRE(mesh->cellCount() == 10);

    auto edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(*mesh);

    std::vector<std::array<int, 2>> non_manifold_edges = inf::SurfaceMesh::selectExposedEdges(*mesh, edge_neighbors);
    REQUIRE(non_manifold_edges.size() == 4);

    std::vector<std::vector<int>> loops = inf::SurfaceMesh::identifyLoops(non_manifold_edges);
    REQUIRE(loops.size() == 1);
    REQUIRE(loops[0].size() == 4);

    mesh = inf::SurfaceMesh::fillHoles(*mesh, loops);
    REQUIRE(mesh->cellCount() == 11);
}

TEST_CASE("Can fill holes on a large gap"){
    auto mesh = inf::CartMesh::create(10, 10, 10);
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto facets = inf::extractOwned2DFacets(mp, *mesh);
    facets.resize(facets.size()-1);
    facets.resize(facets.size()-1);
    facets.resize(facets.size()-1);
    facets.resize(facets.size()-1);
    mesh = inf::STLConverters::stitchSTLToInfinity(facets);
    auto edge_neighbors = inf::SurfaceEdgeNeighbors::buildSurfaceEdgeNeighbors(*mesh);
    std::vector<std::array<int, 2>> non_manifold_edges = inf::SurfaceMesh::selectExposedEdges(*mesh, edge_neighbors);
    REQUIRE(non_manifold_edges.size() == 6);
    std::vector<std::vector<int>> loops = inf::SurfaceMesh::identifyLoops(non_manifold_edges);
    REQUIRE(loops.size() == 1);
    REQUIRE(loops.front().size() == 6);
    mesh = inf::SurfaceMesh::fillHoles(*mesh, loops);
}
