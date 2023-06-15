#include "HoleCutStatPrinter.h"
#include "DruyorTypeAssignment.h"

namespace YOGA {
DruyorTypeAssignment::StatusCounts printStats(const YogaMesh& mesh,
                const std::vector<NodeStatus>& node_statuses,
                const RootPrinter& rootPrinter,
                MessagePasser mp) {
    DruyorTypeAssignment::StatusCounts counts;
    for (int i = 0; i < mesh.nodeCount(); ++i) {
        if (mp.Rank() != mesh.nodeOwner(i)) continue;
        auto s = node_statuses[i];
        if (InNode == s)
            counts.in++;
        else if (OutNode == s)
            counts.out++;
        else if (FringeNode == s)
            counts.receptor++;
        else if (Orphan == s)
            counts.orphan++;
    }
    counts.in = mp.ParallelSum(counts.in);
    counts.out = mp.ParallelSum(counts.out);
    counts.receptor = mp.ParallelSum(counts.receptor);
    counts.orphan = mp.ParallelSum(counts.orphan);

    rootPrinter.print("Yoga:");
    rootPrinter.print(" in: " + std::to_string(counts.in));
    rootPrinter.print(" out: " + std::to_string(counts.out));
    rootPrinter.print(" receptor: " + std::to_string(counts.receptor));
    rootPrinter.print(" orphan: " + std::to_string(counts.orphan));
    rootPrinter.print("\n");
    return counts;
}
}
