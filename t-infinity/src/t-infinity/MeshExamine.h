#pragma once
#include <MessagePasser/MessagePasser.h>
#include "MeshInterface.h"
#include <parfait/ToString.h>

namespace inf {
namespace MeshExamine {
    template <typename Callback>
    void doInRankOrder(MessagePasser mp, Callback callback) {
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            if (mp.Rank() == r) {
                callback();
            }
            mp.Barrier();
        }
    }
    inline void printCells(MessagePasser mp, const inf::MeshInterface& mesh) {
        auto print = [&]() {
            printf("Rank %d\n", mp.Rank());
            printf("Number of Cells %d\n", mesh.cellCount());
            for (int c = 0; c < mesh.cellCount(); c++) {
                printf("Cell %d, %s, GID: %ld, owner: %d\n",
                       c,
                       mesh.cellTypeString(mesh.cellType(c)).c_str(),
                       mesh.globalCellId(c),
                       mesh.cellOwner(c));
            }
        };

        doInRankOrder(mp, print);
    }
    inline void printCounts(MessagePasser mp, const inf::MeshInterface& mesh) {
        std::map<inf::MeshInterface::CellType, long> cell_counts;
        std::set<inf::MeshInterface::CellType> types;
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.cellOwner(c) == mesh.partitionId()) {
                auto type = mesh.cellType(c);
                cell_counts[type]++;
                types.insert(type);
            }
        }

        long node_count = 0;
        for (int n = 0; n < mesh.nodeCount(); n++) {
            if (mesh.nodeOwner(n) == mesh.partitionId()) {
                node_count++;
            }
        }
        node_count = mp.ParallelSum(node_count);
        types = mp.ParallelUnion(types);

        long total_cells = 0;
        for (auto t : types) {
            cell_counts[t] = mp.ParallelSum(cell_counts[t]);
            total_cells += cell_counts[t];
        }

        mp_rootprint("Nodes in grid      : %s\n",
                     Parfait::bigNumberToStringWithCommas(node_count).c_str());
        mp_rootprint("Total cells in grid: %s\n",
                     Parfait::bigNumberToStringWithCommas(total_cells).c_str());
        for (auto& pair : cell_counts) {
            mp_rootprint("%s: %s\n",
                         MeshInterface::cellTypeString(pair.first).c_str(),
                         Parfait::bigNumberToStringWithCommas(pair.second).c_str());
        }
    }
}
}
