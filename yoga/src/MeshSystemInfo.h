#pragma once

#include <MessagePasser/MessagePasser.h>
#include <vector>
#include "PartitionInfo.h"

namespace YOGA {
class MeshSystemInfo {
  public:
    MeshSystemInfo(MessagePasser mp);
    MeshSystemInfo(MessagePasser mp, const PartitionInfo& info);

    int numberOfBodies() const;
    int getComponentIdForBody(int i) const;
    int numberOfComponents() const;
    int numberOfPartitions() const;
    Parfait::Extent<double> getBodyExtent(int id) const;
    Parfait::Extent<double> getComponentExtent(int id) const;
    bool doesOverlapPartitionExtent(const Parfait::Extent<double>& e,int id) const;

  protected:
    MessagePasser mp;
    std::vector<int> componentIdsForBodies;
    std::vector<Parfait::Extent<double>> body_extents;
    std::vector<Parfait::Extent<double>> component_extents;
    std::vector<Parfait::Extent<double>> partition_extents;
    std::vector<std::vector<bool>> partition_masks;

  private:
    std::vector<int> getUniqueIds(const std::vector<int>& ids_on_partition);
    std::vector<Parfait::Extent<double>> parallelCombine(const std::vector<Parfait::Extent<double>>& extents);
    void buildBodyExtents(const PartitionInfo& info);
    void buildComponentExtents(const PartitionInfo& info);
    void buildPartitionExtents(const PartitionInfo& info);
    int countComponents(const PartitionInfo& info);
    std::vector<int> buildComponentIdsForBodies(const PartitionInfo& info);
};
}
