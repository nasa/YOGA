#pragma once
#include <t-infinity/MeshInterface.h>
#include <parfait/LinearPartitioner.h>
#include <parfait/Point.h>
#include <map>
#include <vector>

class NaiveMesh {
  public:
    NaiveMesh(const std::vector<Parfait::Point<double>>& coords,
              const std::map<inf::MeshInterface::CellType, std::vector<long>>& cells,
              const std::map<inf::MeshInterface::CellType, std::vector<int>>& cell_tags,
              const std::map<inf::MeshInterface::CellType, std::vector<long>>& global_cell_ids,
              const std::vector<long>& global_node_id,
              const Parfait::LinearPartitioner::Range<long>& node_range);
    void cell(inf::MeshInterface::CellType type, int cell_id, long* cell_out) const;
    bool doOwnNode(long global) const;
    int ownedCount() const;
    std::vector<inf::MeshInterface::CellType> cellTypes() const;
    long cellCount(inf::MeshInterface::CellType cell_type) const;

  public:
    std::vector<Parfait::Point<double>> xyz;
    std::map<inf::MeshInterface::CellType, std::vector<long>> cells;
    std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
    std::map<inf::MeshInterface::CellType, std::vector<long>> global_cell_ids;
    std::vector<long> local_to_global_node;
    std::map<long, int> global_to_local_node;
    Parfait::LinearPartitioner::Range<long> node_range;
};
