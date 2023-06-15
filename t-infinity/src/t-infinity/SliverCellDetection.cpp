#include "SliverCellDetection.h"

using namespace inf;

std::vector<double> SliverCellDetection::calcWorstCellVolumeRatio(
    const MeshInterface& mesh, const std::vector<std::vector<int>>& c2c) {
    std::vector<double> volume_ratio(mesh.cellCount(), 1.0);
    std::vector<double> cell_volume(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        inf::Cell cell(mesh, c);
        cell_volume[c] = std::fabs(cell.volume());
    }

    for (int c = 0; c < mesh.cellCount(); c++) {
        int dimension = mesh.cellDimensionality(c);
        for (auto& neighbor : c2c[c]) {
            // skip non-existent neighbors
            // For example if c2c is based on face neighbors who might be off rank.
            if (neighbor < 0) continue;

            // don't compare "volumes" between two cells that aren't both volume.
            // (Or any other dimensionality mismatch
            if (mesh.cellDimensionality(neighbor) != dimension) continue;
            double vr = cell_volume[c] / cell_volume[neighbor];
            volume_ratio[c] = std::min(vr, volume_ratio[c]);
        }
    }

    return volume_ratio;
}
