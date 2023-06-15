#include <MessagePasser/MessagePasser.h>
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/TinfMeshAppender.h>
#include <t-infinity/AddHalo.h>
#include <RingAssertions.h>


TEST_CASE("Add halo nodes", "[add-halo]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    inf::TinfMeshData mesh_data;
    if (mp.Rank() == 0) {
        mesh_data.cells[inf::MeshInterface::BAR_2] = {0, 1, 1, 2};
        mesh_data.cell_tags[inf::MeshInterface::BAR_2] = {0, 0, 0};
        mesh_data.cell_owner[inf::MeshInterface::BAR_2] = {0, 1};
        mesh_data.node_owner = {0, 0, 1};
        mesh_data.global_node_id = {100, 101, 102};
        mesh_data.global_cell_id[inf::MeshInterface::BAR_2] = {200, 201};
        mesh_data.points = {{0, 0, 0}, {1, 1, 1}, {2, 2, 2}};
    } else {
        mesh_data.cells[inf::MeshInterface::BAR_2] = {0, 1, 1, 2};
        mesh_data.cell_tags[inf::MeshInterface::BAR_2] = {0, 0, 0};
        mesh_data.cell_owner[inf::MeshInterface::BAR_2] = {1, 1};
        mesh_data.node_owner = {0, 1, 1};
        mesh_data.global_node_id = {101, 102, 103};
        mesh_data.global_cell_id[inf::MeshInterface::BAR_2] = {201, 202};
        mesh_data.points = {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}};
    }

    auto mesh = std::make_shared<inf::TinfMesh>(mesh_data, mp.Rank());

    auto halo_mesh = inf::addHaloNodes(mp, *mesh);

    REQUIRE(halo_mesh->nodeCount() == 4);
}
TEST_CASE("Add halo cells", "[add-halo]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    inf::TinfMeshData mesh_data;
    if (mp.Rank() == 0) {
        mesh_data.cells[inf::MeshInterface::BAR_2] = {0, 1, 1, 2, 2, 3};
        mesh_data.cell_tags[inf::MeshInterface::BAR_2] = {0, 0, 0, 0};
        mesh_data.cell_owner[inf::MeshInterface::BAR_2] = {0, 0, 1};
        mesh_data.node_owner = {0, 0, 0, 1};
        mesh_data.global_node_id = {100, 101, 102, 103};
        mesh_data.global_cell_id[inf::MeshInterface::BAR_2] = {200, 201, 202};
        mesh_data.points = {{0, 0, 0}, {1, 1, 1}, {2, 2, 2}, {3,3,3}};
    } else {
        mesh_data.cells[inf::MeshInterface::BAR_2] = {0, 1, 1, 2, 2, 3};
        mesh_data.cell_tags[inf::MeshInterface::BAR_2] = {0, 0, 0, 0};
        mesh_data.cell_owner[inf::MeshInterface::BAR_2] = {0, 1, 1};
        mesh_data.node_owner = {0, 0, 1, 1};
        mesh_data.global_node_id = {101, 102, 103, 104};
        mesh_data.global_cell_id[inf::MeshInterface::BAR_2] = {201, 202, 203};
        mesh_data.points = {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}, {4,4,4}};
    }

    auto mesh = std::make_shared<inf::TinfMesh>(mesh_data, mp.Rank());

    auto halo_mesh = inf::addHaloCells(mp, *mesh);

    REQUIRE(halo_mesh->nodeCount() == 5);
    REQUIRE(halo_mesh->cellCount() == 4);
}

TEST_CASE("Add halo nodes but only for a set", "[add-halo]"){
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto mesh = inf::CartMesh::create(mp, 4, 4, 1);

    std::set<int> augment_nodes;
    if(mp.Rank() == 0){
        augment_nodes.insert(6);
    }

    auto halo_mesh = inf::addHaloNodes(mp, *mesh, augment_nodes);
    if(mp.Rank() == 0){
        REQUIRE(halo_mesh->nodeCount() == 44);
    }
    if(mp.Rank() == 1){
        REQUIRE(halo_mesh->nodeCount() == 40);
    }
}
