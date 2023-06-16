#include <utility>

#include <utility>

#pragma once

#include <MessagePasser/MessagePasser.h>
#include <t-infinity/TinfMesh.h>
#include <memory>
#include "CellRedistributor.h"
#include "NaiveMesh.h"

class Redistributor {
  public:
    Redistributor(MessagePasser mp, NaiveMesh&& nm, std::vector<int>&& part)
        : mp(mp), naive_mesh(std::move(nm)), partition_vector(std::move(part)) {}
    std::shared_ptr<inf::TinfMesh> createMesh();

  public:
    MessagePasser mp;
    NaiveMesh naive_mesh;
    std::vector<int> partition_vector;

    std::vector<long> redistributeNodes();
    CellCollection redistributeCells(const std::vector<long>& global_nodes);

    std::vector<long> determineResidentNodes(const std::map<inf::MeshInterface::CellType, std::vector<long>>& cells);

    std::pair<std::vector<Parfait::Point<double>>, std::vector<int>> gatherAllResidentCoordinates(
        const std::vector<long>& resident_node_ids);

    std::map<long, int> buildGlobalToLocal(const std::vector<long>& resident_node_ids) const;

    std::map<inf::MeshInterface::CellType, std::vector<int>> mapCellsToLocal(
        const std::map<inf::MeshInterface::CellType, std::vector<long>>& cells_global,
        const std::map<long, int>& global_to_local) const;
    std::map<inf::MeshInterface::CellType, std::vector<int>> determineCellOwner(
        const std::map<inf::MeshInterface::CellType, std::vector<int>>& cells,
        const std::vector<long>& local_to_global,
        const std::vector<int>& vector);
    std::vector<long> getGlobalNodeIdsForCell(const std::vector<long>& global_to_local_node_ids,
                                              const std::vector<int>& cell);
    static std::vector<int> getCellNodeOwners(const std::vector<int>& cell, const std::vector<int>& node_owners);
    static std::vector<int> determineNodeOwners(MessagePasser mp,
                                                long max_owned_id,
                                                const std::vector<long>& global_node_ids);

    void fulfillRequest(const std::vector<long>& requested_gids,
                        std::vector<long>& gids,
                        std::vector<Parfait::Point<double>>& points,
                        std::vector<int>& node_owners);
};
