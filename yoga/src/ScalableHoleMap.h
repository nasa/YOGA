#pragma once

#include <MessagePasser/MessagePasser.h>
#include <parfait/CartBlock.h>
#include <parfait/Extent.h>
#include <Tracer.h>
#include <vector>
#include "CartBlockFloodFill.h"
#include "MeshSystemInfo.h"
#include "PartitionInfo.h"
#include "SymmetryFinder.h"
#include "YogaMesh.h"
#include "CartBlockGenerator.h"

namespace YOGA {

class ScalableHoleMap {
  public:
    ScalableHoleMap(
        MessagePasser mp, const YogaMesh& m, const PartitionInfo& info, const MeshSystemInfo& info2, int indexOfBody, int max_cells)
        : mp(mp),
          associatedComponentId(info2.getComponentIdForBody(indexOfBody)),
          mesh(m),
          partition_info(info),
          block(generateCartBlock(info2.getBodyExtent(indexOfBody),max_cells)),
          cell_statuses(block.numberOfCells(), CartBlockFloodFill::Untouched) {
        Tracer::begin("blank");
        blankLocally();
        Tracer::end("blank");
        Tracer::begin("sync");
        sync();
        Tracer::end("sync");
        Tracer::begin("flood");
        floodFill();
        Tracer::end("flood");
    }

    bool doesOverlapHole(Parfait::Extent<double>& e) const;
    int getAssociatedComponentId() const { return associatedComponentId; }

  //private:
    MessagePasser mp;
    int associatedComponentId;
    const YogaMesh& mesh;
    const PartitionInfo& partition_info;
    Parfait::CartBlock block;
    std::vector<int> cell_statuses;

    void blankLocally();
    void floodFill();
    void blankWithExtent(const Parfait::Extent<double>& e);
    void sync();
};
}
