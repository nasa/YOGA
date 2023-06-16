#pragma once
#include <map>
#include <set>
#include <tuple>
#include <vector>
#include "TinfMesh.h"

class TinfMeshAppender {
  public:
    std::pair<inf::MeshInterface::CellType, int> addCell(const inf::TinfMeshCell& cell);
    inf::TinfMeshData& getData();
    const inf::TinfMeshData& getData() const;
    int cellCount() const;

  public:
    std::map<long, int> node_g2l;
    std::set<long> resident_global_cell_ids;
    std::map<long, std::tuple<inf::MeshInterface::CellType, int>> global_cell_id_to_key;
    inf::TinfMeshData mesh_data;

    std::pair<inf::MeshInterface::CellType, int> appendCell(const inf::TinfMeshCell& cell);
    void throwIfIncomingCellIsDifferent(const inf::TinfMeshCell& cell);
};
