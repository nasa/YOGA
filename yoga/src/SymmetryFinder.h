#pragma once
#include <MessagePasser/MessagePasser.h>
#include "SymmetryPlane.h"
#include <limits>
#include "BoundaryConditions.h"
#include "YogaMesh.h"

namespace YOGA {
class SymmetryFinder {
  public:
    SymmetryFinder() = delete;
    SymmetryFinder(MessagePasser mp, const YogaMesh& mesh);
    std::vector<SymmetryPlane> findSymmetryPlanes();

  private:
    MessagePasser mp;
    const YogaMesh& mesh;
    int countComponents();
    std::vector<int> flagComponentsWithSymmetry();
    std::vector<double> locatePlanes(std::vector<int> symmetry_flags);
    int boundaryFaceImesh(int id);
    double getPosition(int node_id, int dimension);
    double calcPlanePosition(int component_id, int symmetry_flag);
};
}
