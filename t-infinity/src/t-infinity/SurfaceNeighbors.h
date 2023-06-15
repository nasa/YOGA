#pragma once
#include <memory>
#include <set>
#include <vector>
#include "TinfMesh.h"

namespace SurfaceNeighbors {
std::vector<std::vector<int>> buildNode2VolumeCell(const std::shared_ptr<inf::TinfMesh>& mesh);

int findNeighbor(const std::shared_ptr<inf::TinfMesh> mesh,
                 const std::vector<std::vector<int>>& n2c,
                 const int* cell,
                 int cell_length);

std::vector<int> buildSurfaceNeighbors(const std::shared_ptr<inf::TinfMesh>& mesh);
}
