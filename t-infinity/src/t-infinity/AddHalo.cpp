#include "AddHalo.h"
#include "GlobalToLocal.h"
#include <t-infinity/MeshInterface.h>
#include "MeshConnectivity.h"
#include "TinfMesh.h"
#include "TinfMeshAppender.h"

std::shared_ptr<inf::MeshInterface> makeMeshFromIncomingFragments(
    const MessagePasser& mp,
    const inf::TinfMesh& mesh,
    std::map<int, MessagePasser::Message>& incoming_fragments) {
    TinfMeshAppender appender;
    for (int c = 0; c < mesh.cellCount(); c++) {
        appender.addCell(mesh.getCell(c));
    }

    for (auto& pair : incoming_fragments) {
        inf::TinfMeshData incoming;
        incoming.unpack(pair.second);
        inf::TinfMesh fragment(incoming, mp.Rank());
        for (int c = 0; c < fragment.cellCount(); c++) {
            appender.addCell(fragment.getCell(c));
        }
    }

    return std::make_shared<inf::TinfMesh>(appender.getData(), mp.Rank());
}

std::shared_ptr<inf::MeshInterface> addHaloNodes(
    MessagePasser mp,
    const inf::MeshInterface& mesh_in,
    const std::map<int, std::vector<long>>& i_need_these_node_stencils) {
    inf::TinfMesh mesh(mesh_in);
    auto they_need_these_node_stencils = mp.Exchange(i_need_these_node_stencils);
    auto n2c = inf::NodeToCell::build(mesh);
    auto g2l_node = inf::GlobalToLocal::buildNode(mesh);

    std::map<int, MessagePasser::Message> fragments_to_send;
    for (auto& pair : they_need_these_node_stencils) {
        int target_rank = pair.first;
        auto& needed_nodes = pair.second;
        TinfMeshAppender appender;
        for (auto global_id : needed_nodes) {
            for (auto c : n2c[g2l_node.at(global_id)]) {
                auto cell = mesh.getCell(c);
                appender.addCell(cell);
            }
        }
        appender.getData().pack(fragments_to_send[target_rank]);
    }

    auto incoming_fragments = mp.Exchange(fragments_to_send);
    return makeMeshFromIncomingFragments(mp, mesh, incoming_fragments);
}

std::shared_ptr<inf::MeshInterface> inf::addHaloNodes(MessagePasser mp,
                                                      const inf::MeshInterface& mesh,
                                                      const std::set<int>& nodes_to_augment) {
    std::map<int, std::vector<long>> i_need_these_node_stencils;
    for (int n : nodes_to_augment) {
        auto owner = mesh.nodeOwner(n);
        if (owner != mesh.partitionId()) {
            i_need_these_node_stencils[owner].push_back(mesh.globalNodeId(n));
        }
    }
    return addHaloNodes(mp, mesh, i_need_these_node_stencils);
}

std::shared_ptr<inf::MeshInterface> inf::addHaloNodes(MessagePasser mp,
                                                      const inf::MeshInterface& mesh) {
    std::map<int, std::vector<long>> i_need_these_node_stencils;
    for (int n = 0; n < mesh.nodeCount(); n++) {
        auto owner = mesh.nodeOwner(n);
        if (owner != mesh.partitionId()) {
            i_need_these_node_stencils[owner].push_back(mesh.globalNodeId(n));
        }
    }
    return addHaloNodes(mp, mesh, i_need_these_node_stencils);
}
std::shared_ptr<inf::MeshInterface> inf::addHaloCells(MessagePasser mp,
                                                      const inf::MeshInterface& mesh_in) {
    inf::TinfMesh mesh(mesh_in);
    std::map<int, std::vector<long>> i_need_these_cell_stencils;
    for (int c = 0; c < mesh.cellCount(); c++) {
        auto owner = mesh.cellOwner(c);
        if (owner != mesh.partitionId()) {
            i_need_these_cell_stencils[owner].push_back(mesh.globalCellId(c));
        }
    }

    auto they_need_these_cell_stencils = mp.Exchange(i_need_these_cell_stencils);
    auto n2c = inf::NodeToCell::build(mesh);
    auto g2l_cell = inf::GlobalToLocal::buildCell(mesh);

    std::map<int, MessagePasser::Message> fragments_to_send;
    std::vector<int> cell_nodes;
    for (auto& pair : they_need_these_cell_stencils) {
        int target_rank = pair.first;
        auto& needed_cells = pair.second;
        TinfMeshAppender appender;
        for (auto global_id : needed_cells) {
            auto local_cell_id = g2l_cell.at(global_id);
            cell_nodes.resize(mesh.cellTypeLength(mesh.cellType(local_cell_id)));
            mesh.cell(local_cell_id, cell_nodes.data());

            for (auto node : cell_nodes) {
                for (auto neighbor_cell : n2c[node]) {
                    auto cell = mesh.getCell(neighbor_cell);
                    appender.addCell(cell);
                }
            }
        }
        appender.getData().pack(fragments_to_send[target_rank]);
    }

    auto incoming_fragments = mp.Exchange(fragments_to_send);

    return makeMeshFromIncomingFragments(mp, mesh, incoming_fragments);
}
