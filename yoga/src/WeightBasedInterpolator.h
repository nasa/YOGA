#pragma once
#include <InverseReceptor.h>
#include <algorithm>
#include "Interpolator.h"
#include <map>
#include <vector>
#include "YogaMesh.h"
#include "DonorCloud.h"
#include "InterpolationTools.h"
#include "YogaTypeSupport.h"

namespace YOGA {

inline void calcWeightsWithFiniteElements(double* xyz, int n, double* vertices, double* weights) {
    using FEInterpolator = FiniteElementInterpolation<double>;
    switch (n) {
        case 4:
            FEInterpolator::calcWeightsTet(vertices, xyz, weights);
            break;
        case 5:
            FEInterpolator::calcWeightsPyramid(vertices, xyz, weights);
            break;
        case 6:
            FEInterpolator::calcWeightsPrism(vertices, xyz, weights);
            break;
        case 8:
            FEInterpolator::calcWeightsHex(vertices, xyz, weights);
            break;
    }
}

inline void calcWeightsWithLeastSquares(double* xyz, int n, double* vertices, double* weights) {
    LeastSquaresInterpolation<double>::calcWeights(n,vertices,xyz,weights);
}


template<typename WeightCalculator,typename T>
std::map<int,std::vector<DonorCloud>> generateDonorCloudsForInverseReceptors(const YogaMesh& mesh,
                                                                             const std::map<int, std::vector<InverseReceptor<T>>>& inverse_receptors,
                                                                             WeightCalculator weight_calculator){
    std::map<int, std::vector<DonorCloud>> donor_clouds;
    std::vector<Parfait::Point<double>> vertices;
    for (auto& pair : inverse_receptors) {
        int rank = pair.first;
        for (auto& inverse_receptor : pair.second) {
            int cell_id = inverse_receptor.donor_cell_id;
            int n = mesh.numberOfNodesInCell(cell_id);
            DonorCloud cloud(n);
            mesh.getNodesInCell(cell_id, cloud.node_ids.data());
            vertices.resize(n);
            for(int i=0;i<n;i++){
                vertices[i] = mesh.getNode<double>(cloud.node_ids[i]);
            }
            auto p = inverse_receptor.p;
            weight_calculator((double*)p.data(), n, (double*)vertices.front().data(),(double*)cloud.weights.data());
            donor_clouds[rank].push_back(cloud);
        }
    }
    return donor_clouds;
}

template<typename T>
class WeightBasedInterpolator : public Interpolator<T> {
  public:
    template <typename WeightCalculator>
    WeightBasedInterpolator(int n_variables,
                            const std::map<int, std::vector<YOGA::InverseReceptor<T>>>& inverse_receptors,
                            const YogaMesh& mesh,
                            WeightCalculator weight_calculator)
        : nvariables(n_variables) {
        donor_clouds = generateDonorCloudsForInverseReceptors(mesh,inverse_receptors,weight_calculator);
    }
    void interpolate(int rank, int index, std::function<void(int, double*)> getSolutionAtNode, double* q) const {
        throwIfNoDonorCloud(rank, index);
        std::vector<double> tmp(nvariables);
        auto& cloud = donor_clouds.at(rank).at(index);
        std::fill(q, q + nvariables, 0.0);
        for (size_t i = 0; i < cloud.node_ids.size(); i++) {
            int id = cloud.node_ids.at(i);
            double w = cloud.weights.at(i);
            getSolutionAtNode(id, tmp.data());
            std::transform(q, q + nvariables, tmp.begin(), q, [&](double a, double b) { return a + b * w; });
        }
    }

  private:
    const int nvariables;
    std::map<int, std::vector<DonorCloud>> donor_clouds;
    void throwIfNoDonorCloud(int rank, int index) const {
        if (donor_clouds.count(rank) == 0)
            throw std::logic_error("No donorGids from rank " + std::to_string(rank));
        else if (index >= long(donor_clouds.at(rank).size()))
            throw std::logic_error("Asked for more donorGids than possible");
    }
};
}
