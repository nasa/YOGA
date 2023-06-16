#include <RingAssertions.h>
#include <map>
#include <vector>
#include "InterpolationTools.h"
#include "InverseReceptor.h"
#include "WeightBasedInterpolator.h"
#include "YogaMesh.h"

auto generateMockDonorWeightMesh() {
    YOGA::YogaMesh mesh;
    mesh.setNodeCount(4);
    mesh.setCellCount(1);
    mesh.setCells([](int) { return 4; },
                  [](int id, int* cell) {
                      for (int i = 0; i < 4; i++) cell[i] = i;
                  });

    auto getNode = [](int id, double* p) {
        switch (id) {
            case 0:
                p[0] = 0.0;
                p[1] = 0.0;
                p[2] = 0.0;
                break;
            case 1:
                p[0] = 1.0;
                p[1] = 0.0;
                p[2] = 0.0;
                break;
            case 2:
                p[0] = 0.0;
                p[1] = 1.0;
                p[2] = 0.0;
                break;
            case 3:
                p[0] = 0.0;
                p[1] = 0.0;
                p[2] = 1.0;
                break;
        }
    };
    mesh.setXyzForNodes(getNode);
    return mesh;
}

using namespace YOGA;

TEST_CASE("Build a weight calculator based on inverse receptors and callbacks") {
    const int N_Variables = 5;
    auto mesh = generateMockDonorWeightMesh();
    std::map<int, std::vector<YOGA::InverseReceptor<double>>> inverse_receptors;
    YOGA::InverseReceptor<double> inverse_receptor(0, 11, {0.0, 0.0, 1.0});
    inverse_receptors[7].push_back(inverse_receptor);

    auto weight_calculator = [&](double* xyz,int n, double* vertices, double* weights) {
        std::array<Parfait::Point<double>,4> points;
        for(size_t i=0;i<points.size();i++){
            points[i] = {vertices[3*i], vertices[3*i+1], vertices[3*i+2]};
        }
        auto coords = BarycentricInterpolation::calculateBarycentricCoordinates(points, xyz);
        for (int i = 0; i < 4; i++) weights[i] = coords[i];
    };

    YOGA::WeightBasedInterpolator<double> interpolator(5, inverse_receptors, mesh, weight_calculator);

    std::array<double, N_Variables> q;
    std::fill(q.begin(), q.end(), 0.0);
    auto getter = [](int node_id, double* q) {
        for (int i = 0; i < N_Variables; i++) q[i] = double(node_id);
    };

    int rank = 0;
    int inverse_receptor_index = 0;
    REQUIRE_THROWS(interpolator.interpolate(rank, inverse_receptor_index, getter, q.data()));
    rank = 7;
    inverse_receptor_index = 3;
    REQUIRE_THROWS(interpolator.interpolate(rank, inverse_receptor_index, getter, q.data()));

    rank = 7;
    inverse_receptor_index = 0;
    interpolator.interpolate(rank, inverse_receptor_index, getter, q.data());

    REQUIRE(3.0 == Approx(q[0]));
}
