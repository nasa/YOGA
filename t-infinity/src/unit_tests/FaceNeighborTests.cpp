#include <RingAssertions.h>
#include "MockMeshes.h"
#include "t-infinity/CartMesh.h"
#include "parfait/ToString.h"
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/MeshPrinter.h>

template <typename T>
bool contains(const std::vector<T>& v, const T& t) {
    for (auto o : v) {
        if (o == t) return true;
    }
    return false;
}

template <typename T>
bool contains(const std::vector<T>& v, const std::vector<T>& list) {
    for (auto l : list) {
        if (not contains(v, l)) return false;
    }
    return true;
}

TEST_CASE("build node to cell") {
    auto single_tet = inf::mock::getSingleTetMesh();
    auto node_to_cell = inf::NodeToCell::build(*single_tet.get());
    REQUIRE(node_to_cell.size() == 4);
    REQUIRE(node_to_cell[0].size() == 4);
    REQUIRE(node_to_cell[1].size() == 4);
    REQUIRE(node_to_cell[2].size() == 4);
    REQUIRE(node_to_cell[3].size() == 4);
    REQUIRE(contains(node_to_cell[0], {0, 1, 3, 4}));
}

TEST_CASE("Build face neighbors") {
    auto single_tet = inf::mock::getSingleTetMesh();
    auto neighbors = inf::FaceNeighbors::build(*single_tet.get());
    REQUIRE(neighbors.size() == 5);
    REQUIRE(neighbors[0].size() == 1);
    REQUIRE(neighbors[1].size() == 1);
    REQUIRE(neighbors[2].size() == 1);
    REQUIRE(neighbors[3].size() == 1);
    REQUIRE(neighbors[4].size() == 4);
    REQUIRE(contains(neighbors[0], 4));
    REQUIRE(contains(neighbors[1], 4));
    REQUIRE(contains(neighbors[2], 4));
    REQUIRE(contains(neighbors[3], 4));
    REQUIRE(contains(neighbors[4], {0, 1, 2, 3}));
}

TEST_CASE("Build face neighbors for a 2D TRI_3/BAR_2 mesh") {
    auto mesh = inf::TinfMesh(inf::mock::triangleWithBar(), 0);
    auto neighbors = inf::FaceNeighbors::buildOnlyForCellsWithDimensionality(mesh, 1);
    CHECK(neighbors.size() == 2);
    CHECK(neighbors[0].size() == 1);
    CHECK(neighbors[1].empty());
    CHECK(std::vector<int>{1} == neighbors[0]);
}

TEST_CASE("Build face neighbors with off rank neighbors") {
    auto single_tet = inf::mock::getSingleTetMeshNoNeighbors();
    auto neighbors = inf::FaceNeighbors::build(*single_tet.get());
    REQUIRE(neighbors.size() == 1);
    auto off = inf::FaceNeighbors::NEIGHBOR_OFF_RANK;
    REQUIRE(neighbors[0].size() == 4);
    REQUIRE(contains(neighbors[0], {off, off, off, off}));
}

TEST_CASE("Can flatten face neighbors to create unique on a rank") {
    int mock_rank = 0;
    auto mesh = inf::TinfMesh(inf::mock::twoTouchingTets(), mock_rank);
    auto face_neighbors = inf::FaceNeighbors::build(mesh);
    auto unique_faces = inf::FaceNeighbors::flattenToGlobals(mesh, face_neighbors);
    REQUIRE(unique_faces.size() == 1);
    REQUIRE(unique_faces[0][0] == 0);
    REQUIRE(unique_faces[0][1] == 1);
}

TEST_CASE("Can select a single face from a vector of an entire cell") {
    std::vector<Parfait::Point<double>> hex_points{
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};

    std::vector<Parfait::Point<double>> bottom_face_points;

    int face_number = 0;
    // test to protect downstream Vulcan use case.
    inf::FaceNeighbors::getFace(inf::MeshInterface::HEXA_8, hex_points, face_number, bottom_face_points);
    REQUIRE(bottom_face_points[0].approxEqual({0, 0, 0}));
    REQUIRE(bottom_face_points[1].approxEqual({0, 1, 0}));
    REQUIRE(bottom_face_points[2].approxEqual({1, 1, 0}));
    REQUIRE(bottom_face_points[3].approxEqual({1, 0, 0}));
}

TEST_CASE("Face neighbors will not try to find neighbors of NODE elements"){
    auto mesh = inf::CartMesh::createWithBarsAndNodes(2,2,2);
    REQUIRE_NOTHROW(inf::FaceNeighbors::build(*mesh));
}

TEST_CASE("Find face neighbors in 2D mode with bars and nodes"){
    auto mesh = inf::CartMesh::create2DWithBarsAndNodes(1,1);
    auto face_neighbors = inf::FaceNeighbors::buildInMode(*mesh, 2);
    for(int c = 0; c < mesh->cellCount(); c++){
        auto type = mesh->cellType(c);
        switch(type){
            case inf::MeshInterface::BAR_2:{
                REQUIRE(face_neighbors[c].size() == 1);
                break;
            }
            case inf::MeshInterface::QUAD_4:{
                REQUIRE(face_neighbors[c].size() == 4);
                break;
            }
            case inf::MeshInterface::NODE:{
                REQUIRE(face_neighbors[c].size() == 0);
                break;
            }
            default:
                PARFAIT_THROW("There is a weird cell in test mesh");
        }
        for(auto n : face_neighbors[c]){
            if(n < 0) {
                printf("cell %d type %s has neighbor %d\n", c, mesh->cellTypeString(type).c_str(), n);
                printf("cell nodes %s\n", Parfait::to_string(mesh->cell(c)).c_str());
            }
            REQUIRE(n >= 0);
            REQUIRE(n < mesh->cellCount());
        }
    }
}
