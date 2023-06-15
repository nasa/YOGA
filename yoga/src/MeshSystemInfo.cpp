#include "MeshSystemInfo.h"
#include <Tracer.h>
#include <parfait/CartBlock.h>
#include <parfait/ExtentWriter.h>
#include <parfait/ParallelExtent.h>
#include <algorithm>

namespace YOGA {
MeshSystemInfo::MeshSystemInfo(MessagePasser mp) : mp(mp) {}

MeshSystemInfo::MeshSystemInfo(MessagePasser mp, const PartitionInfo& info)
    : mp(mp), componentIdsForBodies(buildComponentIdsForBodies(info)) {
    Tracer::begin("body extents");
    buildBodyExtents(info);
    Tracer::end("body extents");
    Tracer::begin("component extents");
    buildComponentExtents(info);
    Tracer::end("component extents");
    Tracer::begin("partition extents");
    buildPartitionExtents(info);
    Tracer::end("partition extents");

    Tracer::begin("create local mask");
    int n = 8;
    Parfait::CartBlock block(partition_extents[mp.Rank()],n,n,n);
    std::vector<char> mask(n*n*n,false);
    for(int i=0;i<info.numberOfCells();i++){
        auto e = info.getExtentForCell(i);
        int cell_grid_id = info.getAssociatedComponentIdForCell(i);
        bool cell_is_in_overlap = false;
        for(int grid_id=0;grid_id<long(component_extents.size());grid_id++){
            if(grid_id != cell_grid_id and e.intersects(component_extents[grid_id]))
                cell_is_in_overlap = true;
        }

        if(cell_is_in_overlap) {
            auto ids = block.getCellIdsInExtent(e);
            for (int id : ids) mask[id] = true;
        }
    }
    Tracer::end("create local mask");

    Tracer::begin("gather masks");
    auto masks_from_ranks = mp.Gather(mask);
    Tracer::end("gather masks");
    Tracer::begin("convert to bools");
    for(size_t i=0;i<partition_extents.size();i++){
        partition_masks.emplace_back(std::vector<bool>(masks_from_ranks[i].begin(),masks_from_ranks[i].end()));
    }
    Tracer::end("convert to bools");
}

int MeshSystemInfo::numberOfBodies() const { return componentIdsForBodies.size(); }

int MeshSystemInfo::numberOfComponents() const { return component_extents.size(); }

int MeshSystemInfo::numberOfPartitions() const { return partition_extents.size(); }

int MeshSystemInfo::getComponentIdForBody(int i) const { return componentIdsForBodies[i]; }

Parfait::Extent<double> MeshSystemInfo::getBodyExtent(int id) const { return body_extents[id]; }

Parfait::Extent<double> MeshSystemInfo::getComponentExtent(int id) const { return component_extents[id]; }

bool MeshSystemInfo::doesOverlapPartitionExtent(const Parfait::Extent<double>& e,int id) const {
    if(not e.intersects(partition_extents[id]))
        return false;
    Parfait::CartBlock block(partition_extents[id],8,8,8);
    auto cells_to_check = block.getCellIdsInExtent(e);
    for(int cell:cells_to_check)
        if(partition_masks[id][cell])
            return true;

    return false;
}

std::vector<int> MeshSystemInfo::getUniqueIds(const std::vector<int>& ids_on_partition) {
    std::vector<int> ids;
    mp.Gather(ids_on_partition, ids);
    std::sort(ids.begin(), ids.end());
    ids.erase(unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

void MeshSystemInfo::buildBodyExtents(const PartitionInfo& info) {
    for (int component_id : componentIdsForBodies) {
        auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        if (info.containsBody(component_id)) e = info.getExtentForBody(component_id);
        body_extents.push_back(e);
    }
    body_extents = parallelCombine(body_extents);
}

void MeshSystemInfo::buildComponentExtents(const PartitionInfo& info) {
    int nComponents = countComponents(info);
    for (int i = 0; i < nComponents; i++) {
        auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        if (info.containsComponentMesh(i)) e = info.getExtentForComponent(i);
        component_extents.push_back(e);
    }
    component_extents = parallelCombine(component_extents);
}

void MeshSystemInfo::buildPartitionExtents(const PartitionInfo& info) {
    partition_extents.resize(mp.NumberOfProcesses());
    auto my_extent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    for (auto id : info.getComponentIds())
        Parfait::ExtentBuilder::add(my_extent, info.getExtentForComponent(id));
    mp.Gather(my_extent, partition_extents, 0);
    mp.Broadcast(partition_extents, mp.NumberOfProcesses(), 0);
}

std::vector<int> MeshSystemInfo::buildComponentIdsForBodies(const PartitionInfo& info) {
    auto localBodyIds = info.getBodyIds();
    std::vector<int> allBodyIds;

    Tracer::begin("gather body ids");
    mp.Gather(localBodyIds, allBodyIds, 0);
    Tracer::end("gather body ids");
    std::sort(allBodyIds.begin(), allBodyIds.end());
    allBodyIds.erase(std::unique(allBodyIds.begin(), allBodyIds.end()), allBodyIds.end());
    mp.Broadcast(allBodyIds, 0);
    return allBodyIds;
}

int MeshSystemInfo::countComponents(const PartitionInfo& info) {
    auto localComponentIds = info.getComponentIds();
    std::vector<int> allComponentIds;
    mp.Gather(localComponentIds, allComponentIds, 0);
    std::sort(allComponentIds.begin(), allComponentIds.end());
    allComponentIds.erase(std::unique(allComponentIds.begin(), allComponentIds.end()), allComponentIds.end());
    mp.Broadcast(allComponentIds, 0);

    return allComponentIds.size();
}

std::vector<Parfait::Extent<double>> MeshSystemInfo::parallelCombine(
    const std::vector<Parfait::Extent<double>>& extents) {
    Tracer::begin("parallel combine");
    const int N = extents.size();
    std::vector<double> lo(3 * N), hi(3 * N);
    for (int i = 0; i < N; i++) {
        auto& e = extents[i];
        for (int j = 0; j < 3; j++) {
            lo[3 * i + j] = e.lo[j];
            hi[3 * i + j] = e.hi[j];
        }
    }
    auto parallel_lo = mp.ParallelMin(lo, 0);
    auto parallel_hi = mp.ParallelMax(hi, 0);
    mp.Broadcast(parallel_lo, 0);
    mp.Broadcast(parallel_hi, 0);
    std::vector<Parfait::Extent<double>> return_extents;
    for (int i = 0; i < N; i++) {
        double* a = &parallel_lo[3 * i];
        double* b = &parallel_hi[3 * i];
        return_extents.push_back({{a[0], a[1], a[2]}, {b[0], b[1], b[2]}});
    }
    Tracer::end("parallel combine");
    return return_extents;
}
}
