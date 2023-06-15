#pragma once
#include <tuple>
#include <vector>
#include "Cell.h"
#include "MeshSanityChecker.h"
#include "TinfMesh.h"

namespace inf {
namespace MeshFixer {
    std::shared_ptr<inf::TinfMesh> fixAll(std::shared_ptr<inf::MeshInterface> input_mesh);
    std::tuple<bool, std::vector<int>> tryToUnwindCell(inf::Cell cell);
    std::shared_ptr<inf::TinfMesh> remeshHangingEdges(
        std::shared_ptr<inf::MeshInterface> input_mesh);
}
}
