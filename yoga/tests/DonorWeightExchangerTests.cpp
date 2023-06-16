#include <RingAssertions.h>
#include "DonorWeightExchanger.h"
using namespace YOGA;


YogaMesh buildMockMesh(){
    YogaMesh mesh;
    mesh.setCellCount(1);
    auto setCell = [](int i, int* cell){
        for(int k=0;k<4;k++) cell[k] = k;
    };
    mesh.setCells([](int i){return 4;}, setCell);
    mesh.setNodeCount(4);
    auto setXyz = [](int i, double* p){
        p[0] = p[1] = p[2] = 0.0;
    };
    mesh.setXyzForNodes(setXyz);
    mesh.setOwningRankForNodes([](int i){return 0;});
    return mesh;
}

TEST_CASE("Extract donor node info from inverse receptor"){
    auto mesh = buildMockMesh();

    int donor_cell_id = 0;
    int query_node_id = 99;
    Parfait::Point<double> xyz {0,0,0};

    InverseReceptor<double> ir(donor_cell_id, query_node_id, xyz);

    auto calc_weights = [](const double* xyz, int n,const double* vertices, double* w) {};

    auto donor_points = extractDonorPointsForInverseReceptors<double>(mesh,{ir},calc_weights,0);
    REQUIRE(1 == donor_points.size());
    REQUIRE(query_node_id == donor_points[0].query_point_id);
    REQUIRE(4 == donor_points[0].local_ids.size());
    REQUIRE(4 == donor_points[0].locations.size());
    REQUIRE(4 == donor_points[0].owning_ranks.size());
}