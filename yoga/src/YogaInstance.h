#pragma once
#include <functional>
#include <parfait/CellContainmentChecker.h>
#include "CellContainmentWrapper.h"
#include "OversetData.h"
#include "WeightBasedInterpolator.h"
#include "YogaMesh.h"
#include "FUN3DAdjointData.h"
#include <DomainConnectivityInfo.h>

namespace YOGA {
class YogaInstance {
  public:
    YogaInstance() = delete;
    YogaInstance(MessagePasser mp,
                 YogaMesh&& adapter,
                 int numberOfSolutionVars,
                 std::function<void(int, double, double, double, double*)> getter,
                 std::function<void(int, double*)> getter2,
                 bool is_complex)
        : mp(mp),
          mesh(adapter),
          is_complex(is_complex),
          nSolutionVariables(numberOfSolutionVars),
          getSolutionFromSolverInCellAt(getter),
          getSolutionFromSolverAtNode(getter2),
          doesCellContainNode([&](double* vertices, int nvertices, double* p) {
              return Parfait::CellContainmentChecker::isInCell_c(vertices, nvertices, p);
          }),
          calcWeightsForReceptor(&calcWeightsWithLeastSquares),
          solution_at_nodes(nSolutionVariables * mesh.nodeCount()) {}
    MessagePasser mp;
    YogaMesh mesh;
    bool is_complex;
    std::shared_ptr<OversetData> oversetData = nullptr;
    std::shared_ptr<Interpolator<double>> interpolator = nullptr;
    std::shared_ptr<Interpolator<std::complex<double>>> interpolator_complex = nullptr;
    std::shared_ptr<FUN3DAdjointData<double>> fun3d_adjoint_data = nullptr;
    std::shared_ptr<FUN3DAdjointData<std::complex<double>>> fun3d_adjoint_data_complex = nullptr;
    const int nSolutionVariables;
    const std::function<void(int, double, double, double, double*)> getSolutionFromSolverInCellAt;
    const std::function<void(int, double*)> getSolutionFromSolverAtNode;
    std::function<bool(double*, int, double*)> doesCellContainNode;
    std::function<void(double*, int, double*, double*)> calcWeightsForReceptor;
    std::vector<double> solution_at_nodes;
    std::map<int, std::vector<YOGA::InverseReceptor<double>>> inverse_receptors;
    std::map<int, std::vector<YOGA::InverseReceptor<std::complex<double>>>> inverse_receptors_complex;

  private:
};
}
