#include <RingAssertions.h>
#include "YogaMesh.h"
#include "OverDecomposer.h"

using namespace YOGA;

auto generateTetrahedraAlongXAxis(int ntets){
    std::vector<Parfait::Point<double>> tet_vertices;
    std::vector<std::array<int,4>> tets;
    for(int i=0;i<ntets;i++) {
        double x = i;
        tet_vertices.push_back({x, 0, 0});
        tet_vertices.push_back({x+1, 0, 0});
        tet_vertices.push_back({x, 1, 0});
        tet_vertices.push_back({x, 0, 1});
        tets.push_back({4*i,4*i+1,4*i+2,4*i+3});
    }

    YogaMesh mesh;
    mesh.setNodeCount(tet_vertices.size());
    mesh.setCellCount(tets.size());
    mesh.setXyzForNodes([&](int i,double* p){
      p[0]=tet_vertices[i][0];
      p[1]=tet_vertices[i][1];
      p[2]=tet_vertices[i][2];
    });

    mesh.setOwningRankForNodes([](int i){return 0;});
    mesh.setComponentIdsForNodes([](int i){return 0;});

    mesh.setCells([](int i){return 4;},[&](int i,int* c){
      c[0] = tets[i][0];
      c[1] = tets[i][1];
      c[2] = tets[i][2];
      c[3] = tets[i][3];
    });

    mesh.setFaceCount(0);

    return mesh;
}


TEST_CASE("decompose a partition into grid fragments"){
#if 0
    YogaMesh mesh = generateTetrahedraAlongXAxis(65);
    REQUIRE(65 == mesh.cellCount(YogaMesh::TET));
    REQUIRE((65*4) == mesh.numberOfNodes());

    int target_fragment_node_count = 256;
    auto fragments = OverDecomposer::createFragments(mesh,0,target_fragment_node_count);
    REQUIRE(fragments.size() == 2);
#endif
}
