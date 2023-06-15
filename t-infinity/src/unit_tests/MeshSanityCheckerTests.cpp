#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MeshSanityChecker.h>
#include <t-infinity/Cell.h>
#include <t-infinity/ParallelUgridExporter.h>
#include "MockMeshes.h"

using namespace inf;

TEST_CASE("Check nodes") {
    TinfMeshData mesh_data;
    mesh_data.points.resize(4);
    mesh_data.node_owner = {0, 0, 0, 0};

    SECTION("Throw if global ids are not unique locally") {
        mesh_data.global_node_id = {0, 0, 0, 0};
        int mock_partition_id = 7;
        TinfMesh mesh(mesh_data, mock_partition_id);
        REQUIRE_FALSE(MeshSanityChecker::checkGlobalNodeIds(mesh));
    }

    SECTION("Check global cell ids") {
        MessagePasser mp(MPI_COMM_WORLD);
        int starting_node = 4 * mp.Rank();
        mesh_data.cells[inf::MeshInterface::TETRA_4] = {
            starting_node, starting_node + 1, starting_node + 2, starting_node + 3};
        mesh_data.global_cell_id[inf::MeshInterface::TETRA_4] = {mp.Rank()};

        int mock_partition_id = 7;
        TinfMesh mesh(mesh_data, mock_partition_id);

        MeshSanityChecker::checkGlobalCellIds(mesh);
        REQUIRE_THROWS(MeshSanityChecker::checkCellOwners(mesh));
    }
}

TEST_CASE("Can find open cells") {
    SECTION("Doesn't report perfect good cells as open") {
        int mock_rank = 0;
        auto data = inf::mock::onePrism();
        auto mesh = TinfMesh(data, mock_rank);

        int cell_id = 0;
        auto cell = inf::Cell(mesh, cell_id);
        REQUIRE(MeshSanityChecker::isCellClosed(cell));
    }
    // matt:  I think this is mathematically impossible?
    //        I think any permutation still gives a closed surface integral?
    // SECTION("Reports open cells because of permutation"){
    //    int mock_rank = 0;
    //    auto data = inf::mock::onePrism();
    //    data.cells[inf::MeshInterface::CellType::PENTA_6] = {0,1,2,3,5,4};
    //    auto mesh = TinfMesh(data, mock_rank);

    //    int cell_id = 0;
    //    auto cell = inf::Cell(mesh, cell_id);
    //    REQUIRE(not MeshSanityChecker::isCellClosed(cell));
    //    }
}

TEST_CASE("Can detect twisted quad") {
    std::vector<Parfait::Point<double>> quad = {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}};
    REQUIRE(MeshSanityChecker::isQuadAllignedConsistently(quad));
    std::swap(quad[2], quad[3]);
    REQUIRE(not MeshSanityChecker::isQuadAllignedConsistently(quad));
}

TEST_CASE("Detect wrong winded prism") {
    auto raw_mesh = inf::mock::onePrism();
    raw_mesh.cells[inf::MeshInterface::PENTA_6] = {0, 1, 2, 3, 4, 5};
    int mock_rank = 0;
    SECTION("Don't throw for a perfectly good grid") {
        TinfMesh mesh(raw_mesh, mock_rank);
        REQUIRE(MeshSanityChecker::checkFaceWindings(mesh));
    }
    SECTION("Throw for an inverted prism") {
        raw_mesh.cells[inf::MeshInterface::PENTA_6] = {3, 4, 5, 0, 1, 2};
        TinfMesh mesh(raw_mesh, mock_rank);
        REQUIRE_FALSE(MeshSanityChecker::checkFaceWindings(mesh));
    }
    SECTION("Throw for an inverted prism") {
        raw_mesh.cells[inf::MeshInterface::PENTA_6] = {0, 1, 2, 3, 5, 4};
        TinfMesh mesh(raw_mesh, mock_rank);
        REQUIRE_FALSE(MeshSanityChecker::checkFaceWindings(mesh));
    }
}

TEST_CASE("Can identify hanging edges") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh_data = inf::mock::hangingEdge();
    auto mesh = std::make_shared<inf::TinfMesh>(mesh_data, mp.Rank());

    auto n2c = inf::NodeToCell::build(*mesh);
    auto face_neighbors = inf::FaceNeighbors::build(*mesh, n2c);
    REQUIRE_FALSE(MeshSanityChecker::checkOwnedCellsHaveFaceNeighbors(*mesh, n2c, face_neighbors));

    inf::ParallelUgridExporter::write("hanging-edge", *mesh,  mp);
}
TEST_CASE("can find hanging edge neighborhoods"){
    MessagePasser mp(MPI_COMM_SELF);
    auto data = inf::mock::hangingEdge();
    int mock_rank = 0;
    auto mesh = std::make_shared<inf::TinfMesh>(data, mock_rank);

    std::vector<inf::HangingEdgeNeighborhood> hanging_edge_neighborhoods =
        inf::MeshSanityChecker::findHangingEdges(*mesh);

    REQUIRE(hanging_edge_neighborhoods.size() == 1);
    REQUIRE(hanging_edge_neighborhoods[0].quad_cell == mesh->cellCount()-1);
}
