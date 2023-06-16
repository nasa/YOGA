#pragma once
#include <parfait/Point.h>
#include <vector>
#include "QueryPoint.h"

namespace YOGA {

class DonorPackager {
  public:
    struct Donor {
        int cell_id;
        int owner;
        int component_id;
        double distance;
    };
    template <typename MeshInteractorType>
    static std::vector<std::vector<Donor>> package(const std::vector<QueryPoint>& query_points,
                                                   const std::vector<std::vector<int>>& donor_ids,
                                                   const MeshInteractorType& mesh_interactor,
                                                   int my_rank) {
        std::vector<std::vector<Donor>> donors(query_points.size());
        for (size_t i = 0; i < query_points.size(); i++) {
            for (int cell_id : donor_ids[i]) {
                int owner = mesh_interactor.cellOwner(cell_id);
                if (my_rank == owner) {
                    int component_id = mesh_interactor.componentIdForCell(cell_id);
                    double distance = mesh_interactor.distanceAtPointInCell(cell_id, query_points[i].xyz);
                    donors[i].push_back({cell_id, owner, component_id, distance});
                }
            }
        }
        return donors;
    }
};

}
