#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include "t-infinity/Cell.h"
#include "t-infinity/SliverCellDetection.h"
#include <t-infinity/FaceNeighbors.h>


TEST_CASE("Can identify sliver cells"){
    MessagePasser mp(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 2, 2, 2);

    // Deliberately collapse a hex
    int hex_cell_id = 0;
    for(int c = 0; c < mesh->cellCount(); c++){
        if(inf::MeshInterface::HEXA_8 == mesh->cellType(c)){
            hex_cell_id = c;
            break;
        }
    }
    std::vector<int> cell_nodes;
    mesh->cell(hex_cell_id, cell_nodes);
    for(int i = 0; i < 4; i++){
        auto top_point = mesh->mesh.points[cell_nodes[i]];
        auto bot_point = mesh->mesh.points[cell_nodes[i+4]];
        mesh->mesh.points[cell_nodes[i]] = 0.9999*bot_point + 0.0001*top_point;
    }

    inf::Cell cell(mesh, hex_cell_id);
    REQUIRE(cell.volume() < 1e-4);

    auto face_neighbors = inf::FaceNeighbors::build(*mesh);

    auto cell_volume_ratio = inf::SliverCellDetection::calcWorstCellVolumeRatio(*mesh, face_neighbors);

    int sliver_cell_count = 0;
    double sliver_cell_ratio = 1.0e-4;
    for(auto vr : cell_volume_ratio){
        if(vr < sliver_cell_ratio)
            sliver_cell_count++;
    }
    REQUIRE(sliver_cell_count == 1);
}
