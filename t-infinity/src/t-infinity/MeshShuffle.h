#pragma once
#include <parfait/Throw.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/TinfMeshAppender.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/GlobalToLocal.h>
#include <parfait/SyncPattern.h>
#include <parfait/SyncField.h>

namespace inf {
class MeshShuffle {
  public:
    static std::shared_ptr<inf::TinfMesh> claimCellStencils(MessagePasser mp,
                                                            const inf::MeshInterface& mesh,
                                                            std::set<int> cell_stencils_to_claim);
    static std::shared_ptr<inf::TinfMesh> shuffleNodes(MessagePasser mp,
                                                       const inf::MeshInterface& mesh,
                                                       std::vector<int> new_node_owners);
    static std::shared_ptr<inf::TinfMesh> shuffleCells(MessagePasser mp,
                                                       const inf::MeshInterface& mesh,
                                                       std::vector<int> new_cell_owners);
    static std::vector<int> repartitionNodes4D(MessagePasser mp,
                                               const inf::MeshInterface& mesh,
                                               const std::vector<double>& fourth_dimension);
    static std::vector<int> repartitionNodes(MessagePasser mp,
                                             const inf::MeshInterface& mesh,
                                             const std::vector<double>& node_costs);

    static std::vector<int> repartitionNodes(MessagePasser mp, const inf::MeshInterface& mesh);
    static std::shared_ptr<inf::TinfMesh> extendNodeSupport(const MessagePasser& mp,
                                                            const MeshInterface& mesh);
    static std::shared_ptr<inf::TinfMesh> extendCellSupport(const MessagePasser& mp,
                                                            const MeshInterface& mesh);
    static std::vector<int> repartitionCells(MessagePasser mp,
                                             const inf::MeshInterface& mesh,
                                             const std::vector<double>& cell_costs);
    static std::vector<int> repartitionCells(MessagePasser mp, const inf::MeshInterface& mesh);
    static std::shared_ptr<inf::TinfMesh> repartitionByVolumeCells(MessagePasser mp,
                                                                   const inf::MeshInterface& mesh);
    static std::shared_ptr<inf::TinfMesh> repartitionByVolumeCells(
        MessagePasser mp, const inf::MeshInterface& mesh, const std::vector<double>& cell_cost);
};
}
