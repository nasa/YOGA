#pragma once
#include <MessagePasser/MessagePasser.h>
#include <parfait/CartBlock.h>
#include <parfait/ExtentMeshWrapper.h>
#include <vector>
#include "SymmetryPlane.h"

namespace YOGA {

class CartBlockFloodFill {
  public:
    enum Status { Untouched, OutOfHole, InHole, Crossing };

    static std::vector<int> identifyAndMarkOuterCells(Parfait::CartBlock& block, std::vector<int>& cellStatuses) {
        std::vector<int> cells;
        const int nx = block.numberOfCells_X();
        const int ny = block.numberOfCells_Y();
        const int nz = block.numberOfCells_Z();
        // top and bottom faces
        for (int i = 0; i < nx; ++i) {
            for (int j = 0; j < ny; ++j) {
                pushIdIfUntouched(i, j, 0, block, cellStatuses, cells);
                pushIdIfUntouched(i, j, nz - 1, block, cellStatuses, cells);
            }
        }
        // side faces
        for (int i = 0; i < nx; ++i) {
            for (int k = 1; k < nz - 1; ++k) {
                pushIdIfUntouched(i, 0, k, block, cellStatuses, cells);
                pushIdIfUntouched(i, ny - 1, k, block, cellStatuses, cells);
            }
        }
        // other side faces
        for (int j = 1; j < ny - 1; ++j) {
            for (int k = 1; k < nz - 1; ++k) {
                pushIdIfUntouched(0, j, k, block, cellStatuses, cells);
                pushIdIfUntouched(nx - 1, j, k, block, cellStatuses, cells);
            }
        }
        for (int id : cells) cellStatuses[id] = Status::OutOfHole;
        return cells;
    }

    static void removeSeedsOnPlane(SymmetryPlane& plane,
                                   Parfait::CartBlock& block,
                                   std::vector<int>& seeds,
                                   std::vector<int>& cellStatuses) {
        int direction = plane.getPlane();
        double position = plane.getPosition();
        double pillow = 0.0;
        if (SymmetryPlane::X == direction)
            pillow = 0.5 * block.get_dx();
        else if (SymmetryPlane::Y == direction)
            pillow = 0.5 * block.get_dy();
        else
            pillow = 0.5 * block.get_dz();
        auto planeExtent = Parfait::Extent<double>(block.lo, block.hi);
        planeExtent.lo[direction] = position - pillow;
        planeExtent.hi[direction] = position + pillow;
        for (size_t i = 0; i < seeds.size(); ++i) {
            int cellId = seeds[i];
            if (planeExtent.intersects(block.createExtentFromCell(seeds[i]))) {
                seeds.erase(seeds.begin() + i);
                cellStatuses[cellId] = Untouched;
                --i;
            }
        }
    }

    static void stackFill(std::vector<int>& seeds, Parfait::CartBlock& block, std::vector<int>& cellStatuses) {
        while (not seeds.empty()) {
            int cell = seeds.back();
            seeds.pop_back();
            auto nbrs = getNeighbors(cell, block);
            for (int id : nbrs) {
                if (Status::Untouched == cellStatuses[id]) {
                    cellStatuses[id] = Status::OutOfHole;
                    seeds.push_back(id);
                }
            }
        }
        for (auto& s : cellStatuses)
            if (Status::Untouched == s) s = Status::InHole;
    }

    static std::vector<int> getNeighbors(int cellId, Parfait::CartBlock& block) {
        std::vector<int> nbrs;
        int i, j, k;
        block.convertCellIdTo_ijk(cellId, i, j, k);
        if (0 < i) nbrs.push_back(block.convert_ijk_ToCellId(i - 1, j, k));
        if (0 < j) nbrs.push_back(block.convert_ijk_ToCellId(i, j - 1, k));
        if (0 < k) nbrs.push_back(block.convert_ijk_ToCellId(i, j, k - 1));
        if (block.numberOfCells_X() > i + 1) nbrs.push_back(block.convert_ijk_ToCellId(i + 1, j, k));
        if (block.numberOfCells_Y() > j + 1) nbrs.push_back(block.convert_ijk_ToCellId(i, j + 1, k));
        if (block.numberOfCells_Z() > k + 1) nbrs.push_back(block.convert_ijk_ToCellId(i, j, k + 1));

        return nbrs;
    }

  private:
    static void pushIdIfUntouched(
        int i, int j, int k, Parfait::CartBlock& block, std::vector<int>& cellStatuses, std::vector<int>& cells) {
        int cellId = block.convert_ijk_ToCellId(i, j, k);
        if (Status::Untouched == cellStatuses[cellId]) cells.push_back(cellId);
    }
};

}
