#pragma once

#include <map>
#include "GlobalToLocal.h"
#include "InverseReceptor.h"
#include "OversetData.h"
#include "WeightBasedInterpolator.h"
#include "YogaMesh.h"
#include <Tracer.h>
#include <parfait/PointWriter.h>

namespace YOGA {

template<typename T>
struct ReceptorUpdateRequest {
    ReceptorUpdateRequest() = default;
    ReceptorUpdateRequest(int cellId, int nodeId, const Parfait::Point<T>& pt)
        : containingCellId(cellId), localNodeId(nodeId), p(pt) {}
    int containingCellId;
    int localNodeId;
    Parfait::Point<T> p;
};

template<typename T>
inline std::map<int, std::vector<ReceptorUpdateRequest<T>>> mapPointsToRanks(
    const std::map<int, OversetData::DonorCell>& receptors, const YogaMesh& mesh);

template<typename T>
std::map<int, std::vector<InverseReceptor<T>>> generateInverseReceptors(
    MessagePasser mp, const std::map<int, OversetData::DonorCell>& receptors, const YogaMesh& mesh);

inline void exchangeReceptorUpdates(const MessagePasser& mp,
                             int nvar,
                             const std::map<int, OversetData::DonorCell>& receptors,
                             const std::map<int, std::vector<double>>& receptor_update_solutions,
                             std::vector<double>& solution_at_nodes);

template<typename T>
inline void getUpdatedSolutionsFromInterpolator(MessagePasser mp,
                                         const std::map<int, OversetData::DonorCell>& receptors,
                                         const std::map<int, std::vector<InverseReceptor<T>>> inverse_receptor_map,
                                         int nvar,
                                         const YOGA::Interpolator<T>& interpolator,
                                         const std::function<void(int, double*)>& getSolutionAtNode,
                                         std::vector<double>& solution_at_nodes);

template<typename T>
inline void getUpdatedSolutionsFromSolver(MessagePasser mp,
                                   const YogaMesh& mesh,
                                   const std::map<int, OversetData::DonorCell>& receptors,
                                   const std::map<int, std::vector<InverseReceptor<T>>> inverse_receptor_map,
                                   int nvar,
                                   std::function<void(int, double, double, double, double*)> getter,
                                   std::vector<double>& solution_at_nodes);

}

#include "ReceptorUpdate.hpp"
