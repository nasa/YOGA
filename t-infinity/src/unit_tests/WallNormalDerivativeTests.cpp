#include <RingAssertions.h>
#include "MockMeshes.h"
#include <t-infinity/Shortcuts.h>
#include <t-infinity/NormalGradientPointGeneration.h>

using namespace inf;


TEST_CASE("Wall normals"){
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if(mp.NumberOfProcesses() != 1) return;

    std::string mesh_filename = std::string(GRIDS_DIR) + "lb8.ugrid/2x2_cube.lb8.ugrid";
    auto mesh = inf::shortcut::loadMesh(mp, mesh_filename);

    auto normal_point_locations = WallNormalPoints::generateWallNormalPointsFromNodes(*mesh, {6}, 1.0e-1);
    REQUIRE(normal_point_locations.size() == 9);
    for(auto pair : normal_point_locations){
        int node_id = pair.first;
        REQUIRE((node_id >= 0 and node_id < mesh->nodeCount()));
    }
}

TEST_CASE("Nodes in tags"){
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if(mp.NumberOfProcesses() != 1) return;

    std::string mesh_filename = std::string(GRIDS_DIR) + "lb8.ugrid/2x2_cube.lb8.ugrid";
    auto mesh = inf::shortcut::loadMesh(mp, mesh_filename);

    auto nodes_in_tags = WallNormalPoints::nodesInTags(*mesh, {6});
    REQUIRE(nodes_in_tags.size() == 9);
    for(int i = 0; i < 9; i++){
        REQUIRE(nodes_in_tags.count(i) == 1);
    }

}
