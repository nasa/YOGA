#include <RingAssertions.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/InternalFaceRemover.h>

using namespace inf;


TEST_CASE("Remove internal faces (e.g., a triangle with 2 neighboring 3d elements)"){
    TinfMeshData mesh_data;
    mesh_data.points.resize(5);
    mesh_data.cells[MeshInterface::TRI_3] = {1,2,3};
    mesh_data.cells[MeshInterface::TETRA_4] = {0,1,2,3,1,2,3,4};
    mesh_data.cell_tags[MeshInterface::TRI_3] = {0};
    mesh_data.cell_tags[MeshInterface::TETRA_4] = {0,0};
    mesh_data.global_cell_id[MeshInterface::TRI_3] = {0};
    mesh_data.global_cell_id[MeshInterface::TETRA_4] = {1,2};
    mesh_data.global_node_id = {0,1,2,3,4};
    mesh_data.node_owner = {0,0,0,0,0};
    mesh_data.cell_owner[MeshInterface::TRI_3] = {0};
    mesh_data.cell_owner[MeshInterface::TETRA_4] = {0,0};

    auto mesh = std::make_shared<TinfMesh>(mesh_data,0);

    auto internal_faces = InternalFaceRemover::identifyInternalFaces(*mesh);

    REQUIRE(1 == internal_faces.size());

    MessagePasser mp(MPI_COMM_WORLD);
    int color = 1;
    if(mp.Rank() == 0) color = 0;
    auto root_only_comm = mp.split(mp.getCommunicator(),color);
    if(mp.Rank() == 0) {
        auto mesh2 = InternalFaceRemover::removeCells(root_only_comm, mesh, internal_faces);
        REQUIRE(2 == mesh2->cellCount());
        for(int i=0;i<mesh2->cellCount();i++){
            REQUIRE(MeshInterface::TETRA_4 == mesh2->cellType(i));
        }
    }
}
