#pragma once

#include <t-infinity/MeshInterface.h>
#include <MessagePasser/MessagePasser.h>
#include <functional>
#include <map>
#include <memory>

struct CellCollection {
    std::map<inf::MeshInterface::CellType, std::vector<long>> cells;
    std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
    std::map<inf::MeshInterface::CellType, std::vector<long>> cell_global_ids;
};

class CellRedistributor {
  public:
    CellRedistributor(MessagePasser mp,
                      const std::map<inf::MeshInterface::CellType, std::vector<long>>& cells,
                      const std::map<inf::MeshInterface::CellType, std::vector<int>>& cell_tags,
                      const std::map<inf::MeshInterface::CellType, std::vector<long>>& global_cell_ids);

    CellCollection redistributeCells(std::function<bool(long)> amIResponsibleForNode,
                                     std::function<long(int)> getGlobalNodeId,
                                     const std::vector<int>& future_node_owners);

  public:
    std::vector<std::pair<inf::MeshInterface::CellType, int>> queueCellsContainingNodes(std::vector<long>& nodes);

    std::vector<long> packCells(const std::vector<std::pair<inf::MeshInterface::CellType, int>>& send_cells);

    CellCollection unpackCells(std::vector<std::vector<long>>&& packed_cells_from_other_ranks);

  private:
    MessagePasser mp;
    const std::map<inf::MeshInterface::CellType, std::vector<long>>& cells;
    const std::map<inf::MeshInterface::CellType, std::vector<int>>& cell_tags;
    const std::map<inf::MeshInterface::CellType, std::vector<long>>& global_cell_ids;

    bool shouldSendCell(std::vector<long>& needed_nodes, const long* cell, int cell_length) const;
    std::vector<std::vector<std::pair<inf::MeshInterface::CellType, int>>> assignCellKeysToRanks(
        std::function<bool(long)> amIResponsibleForNode, std::function<int(long)> getNodeFutureOwner);
    std::vector<int> determineWhereToSendCell(std::function<bool(long)> responsibleForNode,
                                              std::function<int(long)> getNodeFutureOwner,
                                              const long* cell,
                                              int length);
};
