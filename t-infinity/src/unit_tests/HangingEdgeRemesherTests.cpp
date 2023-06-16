#include <RingAssertions.h>
#include "../../t-infinity/src/unit_tests/MockMeshes.h"
#include <parfait/Throw.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/MeshBuilder.h>
#include <t-infinity/HangingEdgeRemesher.h>

using namespace inf;

TEST_CASE("Just remesh the whole mesh until there are no hanging edges", "[hang]"){
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = mock::hangingEdge();
    auto tinf_mesh = std::make_shared<inf::TinfMesh>(mesh, 0);

    auto fixed_mesh = HangingEdge::remesh(mp, tinf_mesh);

    inf::shortcut::visualize("fixed-mesh-in-unit-test", MessagePasser(MPI_COMM_WORLD), fixed_mesh);
    REQUIRE(fixed_mesh->mesh.cells[inf::MeshInterface::CellType::QUAD_4].size() / 4 == 9);
    REQUIRE(fixed_mesh->mesh.cells[inf::MeshInterface::CellType::TRI_3].size() / 3 == 2);
    REQUIRE(fixed_mesh->mesh.cells[inf::MeshInterface::CellType::PENTA_6].size() / 6 == 2);
    REQUIRE(fixed_mesh->mesh.cells[inf::MeshInterface::CellType::PYRA_5].size() / 5 == 5);
    REQUIRE(fixed_mesh->mesh.cells[inf::MeshInterface::CellType::HEXA_8].size() == 0);

    REQUIRE(fixed_mesh->cellCount() == 20);
}

TEST_CASE("Identify Next hanging edge quad cell") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    using CellType = MeshInterface::CellType;
    auto mock_mesh = mock::hangingEdge();
    int mock_rank = 0;
    auto tinf_mesh = TinfMesh(mock_mesh, mock_rank);
    auto n2c = NodeToCell::build(tinf_mesh);
    auto face_neighbors = FaceNeighbors::build(tinf_mesh);

    int next_quad_cell = HangingEdge::findNextCellToDelete(tinf_mesh, n2c, face_neighbors);
    auto c2type_and_index = tinf_mesh.mesh.buildCellIdToTypeAndLocalIdMap(tinf_mesh.mesh);
    CellType type;
    int index;
    std::tie(type, index) = c2type_and_index.at(next_quad_cell);
    int length = MeshInterface::cellTypeLength(type);
    std::set<int> cell_nodes;
    for(int i = 0; i < length; i++){
        int n = tinf_mesh.mesh.cells.at(type).at(length*index+i);
        cell_nodes.insert(n);
    }

    REQUIRE(type == CellType::HEXA_8);
    REQUIRE(index == 0);

    inf::experimental::MeshBuilder builder(mp, tinf_mesh);
    builder.deleteCell(type, index);

    auto cells_near_cavity = HangingEdge::getNodeNeighborsOfCell(tinf_mesh, next_quad_cell, n2c);

    auto cavity_faces =
        HangingEdge::getFacesPointingToCellOrMissing(tinf_mesh, c2type_and_index, cells_near_cavity, face_neighbors, cell_nodes, next_quad_cell);
    REQUIRE(cavity_faces.size() == 7);

    builder.steinerCavity(cavity_faces);

    REQUIRE(builder.mesh->mesh.cells[CellType::TETRA_4].size() / 4 == 2);

    builder.sync();

    auto new_mesh = builder.mesh;
    shortcut::visualize("fixed-hanging-edge-with-algorithm", mp, new_mesh);
}

