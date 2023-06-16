#pragma once
#include "FaceNeighbors.h"
#include "VectorFieldAdapter.h"
#include <parfait/VectorTools.h>

namespace inf {
namespace debug {
    inline std::vector<int> highlightCellsMissingNeighbors(const inf::MeshInterface& mesh) {
        auto face_neighbors = inf::FaceNeighbors::build(mesh);
        std::vector<int> bad(mesh.cellCount(), 0);
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (not mesh.ownedCell(c)) continue;
            for (auto n : face_neighbors[c]) {
                if (n == inf::FaceNeighbors::NEIGHBOR_OFF_RANK) bad[c] = 1.0;
            }
        }
        return bad;
    }

    std::shared_ptr<inf::VectorFieldAdapter> highlightCellsMissingNeighborsAsField(
        const inf::MeshInterface& mesh) {
        auto bad = highlightCellsMissingNeighbors(mesh);
        return std::make_shared<inf::VectorFieldAdapter>(
            "missing-neighbor", inf::FieldAttributes::Cell(), Parfait::VectorTools::to_double(bad));
    }
}
}