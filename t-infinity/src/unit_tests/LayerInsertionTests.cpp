#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <parfait/Throw.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/LayerInsertion.h>
#include "MockMeshes.h"

TEST_CASE("Layer insertion "){
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if(mp.NumberOfProcesses() > 1) return;
    auto mesh =  inf::mock::getSingleHexMeshWithFaces(0);

    REQUIRE(mesh->cellCount() == 1*1*1 + 1*1*6);
    REQUIRE(mesh->nodeCount() == 8);

    std::vector<int> cell_nodes(4);
    mesh->cell(0, cell_nodes.data());
    REQUIRE(cell_nodes == std::vector<int>{0,3,2,1});
    cell_nodes.resize(8);
    mesh->cell(6, cell_nodes.data());
    REQUIRE(cell_nodes == std::vector<int>{0,1,2,3,4,5,6,7});

    inf::LayerInsertion::insertLayer(*mesh);
    REQUIRE(mesh->nodeCount() == 16);

    REQUIRE(mesh->cellCount() == 1*1*1 + 1*1*6*2);
    mesh->cell(7, cell_nodes.data());
    REQUIRE(cell_nodes == std::vector<int>{8,11,10,9,0,1,2,3});
//    inf::shortcut::visualize("cart-layer-insertion.lb8.ugrid", mp, mesh, {});
}
