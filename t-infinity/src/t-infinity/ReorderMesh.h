#include <vector>
#include <memory>
#include <t-infinity/MeshInterface.h>
#include <parfait/MapTools.h>
#include <parfait/Throw.h>
#include "TinfMesh.h"
#include "ReverseCutthillMckee.h"
#include "MeshConnectivity.h"
#include "MeshConnectivity.h"

namespace inf {

namespace MeshReorder {
    enum Algorithm { RCM, RANDOM, Q };

    void writeAdjacency(std::string filename, const std::vector<std::vector<int>>& adjacency);

    std::shared_ptr<inf::TinfMesh> reorderCellsRandomly(
        std::shared_ptr<inf::MeshInterface> input_mesh);
    std::shared_ptr<inf::TinfMesh> reorderCellsQ(std::shared_ptr<inf::MeshInterface> input_mesh,
                                                 double p);

    std::string reportEstimatedCacheEfficiencyForCells(MessagePasser mp,
                                                       const inf::MeshInterface& mesh);

    std::shared_ptr<inf::TinfMesh> reorderCellsRCM(std::shared_ptr<inf::MeshInterface> input_mesh);
    std::shared_ptr<inf::TinfMesh> reorderCells(std::shared_ptr<inf::MeshInterface> input_mesh,
                                                Algorithm algorithm = RCM,
                                                double p = 1.0);
    std::shared_ptr<inf::TinfMesh> reorderCells(
        std::shared_ptr<inf::MeshInterface> input_mesh,
        const std::map<inf::MeshInterface::CellType, std::vector<int>>& old_to_new_cell_orderings);

    std::shared_ptr<inf::TinfMesh> reorderNodesRCM(std::shared_ptr<inf::MeshInterface> input_mesh);
    std::shared_ptr<inf::TinfMesh> reorderNodes(std::shared_ptr<inf::MeshInterface> input_mesh,
                                                Algorithm algorithm = RCM,
                                                double p = 1.0);
    std::shared_ptr<inf::TinfMesh> reorderNodes(
        std::shared_ptr<inf::MeshInterface> input_mesh,
        std::function<std::vector<int>(const std::vector<std::vector<int>>&)> graph_reorder);

    std::shared_ptr<inf::TinfMesh> reorderNodesBasedOnCells(
        std::shared_ptr<inf::MeshInterface> input_mesh);

    std::shared_ptr<inf::TinfMesh> reorderMeshNodes(
        std::shared_ptr<inf::MeshInterface> input_mesh,
        const std::vector<int>& old_to_new_node_ordering);

    std::vector<int> buildReorderNodeOwnedFirst(std::vector<bool>& do_own);

}
}
