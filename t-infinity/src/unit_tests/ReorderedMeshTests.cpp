#include <RingAssertions.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/ReorderMesh.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/ReverseCutthillMckee.h>
#include <parfait/MapTools.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/VectorFieldAdapter.h>

TEST_CASE("Reorder nodes") {
    inf::TinfMeshData data;
    data.cells[inf::MeshInterface::TRI_3] = {2, 1, 0};
    data.cell_owner[inf::MeshInterface::TRI_3] = {8};
    data.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    data.node_owner = {0, 0, 1};
    data.global_node_id = {100, 101, 102};
    data.global_cell_id[inf::MeshInterface::TRI_3] = {999};
    data.cell_tags[inf::MeshInterface::TRI_3] = {6662};

    int mock_partition_id = 7;
    auto mesh = std::make_shared<inf::TinfMesh>(data, mock_partition_id);

    std::vector<int> old_to_new_node = {1, 2, 0};
    auto reordered_mesh = inf::MeshReorder::reorderMeshNodes(mesh, {1, 2, 0});

    REQUIRE(reordered_mesh->nodeCount() == mesh->nodeCount());
    for (int old_node = 0; old_node < 3; old_node++) {
        int new_node = old_to_new_node[old_node];
        auto temp = reordered_mesh->node(new_node);
        REQUIRE(temp[0] == data.points[old_node][0]);
        REQUIRE(temp[1] == data.points[old_node][1]);
        REQUIRE(temp[2] == data.points[old_node][2]);
        REQUIRE(mesh->nodeOwner(old_node) == reordered_mesh->nodeOwner(new_node));
        REQUIRE(mesh->globalNodeId(old_node) == reordered_mesh->globalNodeId(new_node));
    }

    std::vector<int> new_tri(3);
    reordered_mesh->cell(0, new_tri.data());
    std::vector<int> reorder_tri = {0, 2, 1};
    REQUIRE(new_tri[0] == reorder_tri[0]);
    REQUIRE(new_tri[1] == reorder_tri[1]);
    REQUIRE(new_tri[2] == reorder_tri[2]);

    REQUIRE(reordered_mesh->globalCellId(0) == 999);
    REQUIRE(reordered_mesh->cellOwner(0) == 8);
    REQUIRE(reordered_mesh->cellTag(0) == 6662);
}

void visualizeCellIds(MessagePasser mp, std::string filename, std::shared_ptr<inf::MeshInterface> mesh) {
    auto cell_id = std::vector<double>(mesh->cellCount());
    for (size_t i = 0; i < cell_id.size(); i++) {
        cell_id[i] = double(i);
    }
    auto f = std::make_shared<inf::VectorFieldAdapter>("cell-id", inf::FieldAttributes::Cell(), 1, cell_id);

    inf::shortcut::visualize(filename, mp, mesh, {f});
}

TEST_CASE("Build c2c for same types only") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    printf("Rank: %i\n", mp.Rank());
    auto mesh = inf::shortcut::loadMesh(mp, std::string(GRIDS_DIR) + "lb8.ugrid/10x10x10_irregular.lb8.ugrid");

    auto reordered_mesh = inf::MeshReorder::reorderCells(mesh);

    visualizeCellIds(mp, "tet-mesh.0", mesh);
    visualizeCellIds(mp, "tet-mesh.1", reordered_mesh);
}

TEST_CASE("Reorder cells by type") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::createVolume(10, 5, 6);

    visualizeCellIds(mp, "cart-mesh.0", mesh);

    std::map<inf::MeshInterface::CellType, std::vector<int>> cell_old_to_new_reordering;
    auto c2c = inf::CellToCell::build(*mesh);
    cell_old_to_new_reordering[inf::MeshInterface::HEXA_8] = inf::ReverseCutthillMckee::calcNewIds(c2c);

    auto reordered_mesh = inf::MeshReorder::reorderCells(mesh, cell_old_to_new_reordering);
    visualizeCellIds(mp, "cart-mesh.1", reordered_mesh);
}
