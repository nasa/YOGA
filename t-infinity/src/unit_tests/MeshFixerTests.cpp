#include <RingAssertions.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/Cell.h>
#include <t-infinity/MeshSanityChecker.h>
#include <t-infinity/MeshFixer.h>
#include "MockMeshes.h"

TEST_CASE("MeshFixer exists") {
    inf::TinfMeshData data;
    data.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}};

    data.cells[inf::MeshInterface::PENTA_6] = {0, 1, 2, 3, 5, 4};
    int mock_rank = 0;
    auto mesh = inf::TinfMesh(data, mock_rank);
    int cell_id = 0;
    inf::Cell cell(mesh, cell_id);

    REQUIRE(not inf::MeshSanityChecker::areCellFacesValid(cell));
    bool valid;
    std::vector<int> new_cell_nodes;
    std::tie(valid, new_cell_nodes) = inf::MeshFixer::tryToUnwindCell(cell);
    REQUIRE(valid);
    data.cells[inf::MeshInterface::PENTA_6] = new_cell_nodes;
    mesh = inf::TinfMesh(data, mock_rank);
    cell = inf::Cell(mesh, cell_id);

    REQUIRE(inf::MeshSanityChecker::areCellFacesValid(cell));
}

TEST_CASE("MeshFixer can fix a whole mesh") {
    MessagePasser mp(MPI_COMM_WORLD);
    inf::TinfMeshData data = inf::mock::onePrism();

    data.cells[inf::MeshInterface::PENTA_6] = {0, 1, 2, 3, 5, 4};

    // make the cell owned by another rank, because if it was
    // owned, it would have to have all of its neighbors
    data.cell_owner[inf::MeshInterface::PENTA_6] = {99};

    int mock_rank = 0;
    auto mesh = std::make_shared<inf::TinfMesh>(data, mock_rank);

    REQUIRE_FALSE(inf::MeshSanityChecker::checkFaceWindings(*mesh));

    auto mesh_shared_ptr = inf::MeshFixer::fixAll(mesh);

    REQUIRE_FALSE(inf::MeshSanityChecker::checkFaceWindings(*mesh));
}



