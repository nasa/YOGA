#include "ElevateMesh.h"
#include "parfait/Checkpoint.h"
#include "Gradients.h"
#include <parfait/MapTools.h>

namespace inf {
namespace P1ToP2 {

    bool hasMidCellPoint(inf::MeshInterface::CellType t) {
        using Type = inf::MeshInterface::CellType;
        switch (t) {
            case Type::QUAD_9:
            case Type::HEXA_27:
                return true;
            default:
                return false;
        }
    }
    bool hasMidQuadFacePoint(inf::MeshInterface::CellType t) {
        using Type = inf::MeshInterface::CellType;
        switch (t) {
            case Type::QUAD_9:
            case Type::PYRA_14:
            case Type::PENTA_18:
            case Type::HEXA_27:
                return true;
            default:
                return false;
        }
    }
    std::vector<Parfait::Point<double>> elevate_Bar_2(
        const std::vector<Parfait::Point<double>>& p1_points,
        MeshInterface::CellType inflated_type) {
        using Type = inf::MeshInterface::CellType;
        switch (inflated_type) {
            case Type::BAR_3:
                return std::vector<Parfait::Point<double>>{
                    p1_points[0], 0.5 * (p1_points[0] + p1_points[1]), p1_points[1]};
            default:
                PARFAIT_THROW("Could not map types: " + MeshInterface::cellTypeString(Type::BAR_2) +
                              " to " + MeshInterface::cellTypeString(inflated_type));
        }
    }

    std::vector<Parfait::Point<double>> elevate_Tri_3(
        const std::vector<Parfait::Point<double>>& p1_points,
        MeshInterface::CellType inflated_type) {
        using Type = inf::MeshInterface::CellType;
        switch (inflated_type) {
            case Type::TRI_6: {
                std::vector<Parfait::Point<double>> out(6);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = 0.5 * (p1_points[0] + p1_points[1]);
                out[4] = 0.5 * (p1_points[1] + p1_points[2]);
                out[5] = 0.5 * (p1_points[2] + p1_points[0]);
                return out;
            }
            default:
                PARFAIT_THROW("Could not map types: " + MeshInterface::cellTypeString(Type::TRI_3) +
                              " to " + MeshInterface::cellTypeString(inflated_type));
        }
    }

    std::vector<Parfait::Point<double>> elevate_QUAD_4(
        const std::vector<Parfait::Point<double>>& p1_points,
        MeshInterface::CellType inflated_type) {
        using Type = inf::MeshInterface::CellType;
        switch (inflated_type) {
            case Type::QUAD_8: {
                std::vector<Parfait::Point<double>> out(8);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = p1_points[3];
                out[4] = 0.5 * (p1_points[0] + p1_points[1]);
                out[5] = 0.5 * (p1_points[1] + p1_points[2]);
                out[6] = 0.5 * (p1_points[2] + p1_points[3]);
                out[7] = 0.5 * (p1_points[3] + p1_points[0]);
                return out;
            }
            case Type::QUAD_9: {
                std::vector<Parfait::Point<double>> out(9);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = p1_points[3];
                out[4] = 0.5 * (p1_points[0] + p1_points[1]);
                out[5] = 0.5 * (p1_points[1] + p1_points[2]);
                out[6] = 0.5 * (p1_points[2] + p1_points[3]);
                out[7] = 0.5 * (p1_points[3] + p1_points[0]);
                out[8] = 0.25 * (p1_points[0] + p1_points[1] + p1_points[2] + p1_points[3]);
                return out;
            }
            default:
                PARFAIT_THROW(
                    "Could not map types: " + MeshInterface::cellTypeString(Type::QUAD_4) + " to " +
                    MeshInterface::cellTypeString(inflated_type));
        }
    }

    std::vector<Parfait::Point<double>> elevate_Tet_4(
        const std::vector<Parfait::Point<double>>& p1_points,
        MeshInterface::CellType inflated_type) {
        using Type = inf::MeshInterface::CellType;
        switch (inflated_type) {
            case Type::TETRA_10: {
                std::vector<Parfait::Point<double>> out(10);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = p1_points[3];

                out[4] = 0.5 * (p1_points[0] + p1_points[1]);
                out[5] = 0.5 * (p1_points[1] + p1_points[2]);
                out[6] = 0.5 * (p1_points[2] + p1_points[0]);

                out[7] = 0.5 * (p1_points[0] + p1_points[3]);
                out[8] = 0.5 * (p1_points[1] + p1_points[3]);
                out[9] = 0.5 * (p1_points[2] + p1_points[3]);
                return out;
            }
            default:
                PARFAIT_THROW(
                    "Could not map types: " + MeshInterface::cellTypeString(Type::TETRA_4) +
                    " to " + MeshInterface::cellTypeString(inflated_type));
        }
    }

    std::vector<Parfait::Point<double>> elevate_Pyra_5(
        const std::vector<Parfait::Point<double>>& p1_points,
        MeshInterface::CellType inflated_type) {
        using Type = inf::MeshInterface::CellType;
        switch (inflated_type) {
            case Type::PYRA_13: {
                std::vector<Parfait::Point<double>> out(13);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = p1_points[3];
                out[4] = p1_points[4];

                out[5] = 0.5 * (p1_points[0] + p1_points[1]);
                out[6] = 0.5 * (p1_points[1] + p1_points[2]);
                out[7] = 0.5 * (p1_points[2] + p1_points[3]);
                out[8] = 0.5 * (p1_points[3] + p1_points[0]);

                out[9] = 0.5 * (p1_points[0] + p1_points[4]);
                out[10] = 0.5 * (p1_points[1] + p1_points[4]);
                out[11] = 0.5 * (p1_points[2] + p1_points[4]);
                out[12] = 0.5 * (p1_points[3] + p1_points[4]);
                return out;
            }
            case Type::PYRA_14: {
                std::vector<Parfait::Point<double>> out(14);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = p1_points[3];
                out[4] = p1_points[4];

                out[5] = 0.5 * (p1_points[0] + p1_points[1]);
                out[6] = 0.5 * (p1_points[1] + p1_points[2]);
                out[7] = 0.5 * (p1_points[2] + p1_points[3]);
                out[8] = 0.5 * (p1_points[3] + p1_points[0]);

                out[9] = 0.5 * (p1_points[0] + p1_points[4]);
                out[10] = 0.5 * (p1_points[1] + p1_points[4]);
                out[11] = 0.5 * (p1_points[2] + p1_points[4]);
                out[12] = 0.5 * (p1_points[3] + p1_points[4]);
                out[13] = 0.25 * (p1_points[0] + p1_points[1] + p1_points[2] + p1_points[3]);
                return out;
            }
            default:
                PARFAIT_THROW(
                    "Could not map types: " + MeshInterface::cellTypeString(Type::PYRA_5) + " to " +
                    MeshInterface::cellTypeString(inflated_type));
        }
    }

    std::vector<Parfait::Point<double>> elevate_Penta_6(
        const std::vector<Parfait::Point<double>>& p1_points,
        MeshInterface::CellType inflated_type) {
        using Type = inf::MeshInterface::CellType;
        switch (inflated_type) {
            case Type::PENTA_15: {
                std::vector<Parfait::Point<double>> out(15);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = p1_points[3];
                out[4] = p1_points[4];
                out[5] = p1_points[5];

                out[6] = 0.5 * (p1_points[0] + p1_points[1]);
                out[7] = 0.5 * (p1_points[1] + p1_points[2]);
                out[8] = 0.5 * (p1_points[2] + p1_points[0]);

                out[9] = 0.5 * (p1_points[0] + p1_points[3]);
                out[10] = 0.5 * (p1_points[1] + p1_points[4]);
                out[11] = 0.5 * (p1_points[2] + p1_points[5]);

                out[12] = 0.5 * (p1_points[3] + p1_points[4]);
                out[13] = 0.5 * (p1_points[4] + p1_points[5]);
                out[14] = 0.5 * (p1_points[5] + p1_points[3]);
                return out;
            }
            case Type::PENTA_18: {
                std::vector<Parfait::Point<double>> out(18);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = p1_points[3];
                out[4] = p1_points[4];
                out[5] = p1_points[5];

                out[6] = 0.5 * (p1_points[0] + p1_points[1]);
                out[7] = 0.5 * (p1_points[1] + p1_points[2]);
                out[8] = 0.5 * (p1_points[2] + p1_points[0]);

                out[9] = 0.5 * (p1_points[0] + p1_points[3]);
                out[10] = 0.5 * (p1_points[1] + p1_points[4]);
                out[11] = 0.5 * (p1_points[2] + p1_points[5]);

                out[12] = 0.5 * (p1_points[3] + p1_points[4]);
                out[13] = 0.5 * (p1_points[4] + p1_points[5]);
                out[14] = 0.5 * (p1_points[5] + p1_points[3]);

                out[15] = 0.25 * (p1_points[0] + p1_points[1] + p1_points[3] + p1_points[4]);
                out[16] = 0.25 * (p1_points[0] + p1_points[2] + p1_points[3] + p1_points[5]);
                out[17] = 0.25 * (p1_points[2] + p1_points[1] + p1_points[5] + p1_points[4]);
                return out;
            }
            default:
                PARFAIT_THROW(
                    "Could not map types: " + MeshInterface::cellTypeString(Type::PYRA_5) + " to " +
                    MeshInterface::cellTypeString(inflated_type));
        }
    }

    std::vector<Parfait::Point<double>> elevate_Hexa_8(
        const std::vector<Parfait::Point<double>>& p1_points,
        MeshInterface::CellType inflated_type) {
        using Type = inf::MeshInterface::CellType;
        switch (inflated_type) {
            case Type::HEXA_20: {
                std::vector<Parfait::Point<double>> out(20);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = p1_points[3];
                out[4] = p1_points[4];
                out[5] = p1_points[5];
                out[6] = p1_points[5];
                out[7] = p1_points[5];

                out[8] = 0.5 * (p1_points[0] + p1_points[1]);
                out[9] = 0.5 * (p1_points[1] + p1_points[2]);
                out[10] = 0.5 * (p1_points[2] + p1_points[3]);
                out[11] = 0.5 * (p1_points[3] + p1_points[0]);

                out[12] = 0.5 * (p1_points[0] + p1_points[4]);
                out[13] = 0.5 * (p1_points[1] + p1_points[5]);
                out[14] = 0.5 * (p1_points[2] + p1_points[6]);
                out[15] = 0.5 * (p1_points[3] + p1_points[7]);

                out[16] = 0.5 * (p1_points[4] + p1_points[5]);
                out[17] = 0.5 * (p1_points[5] + p1_points[6]);
                out[18] = 0.5 * (p1_points[6] + p1_points[7]);
                out[19] = 0.5 * (p1_points[7] + p1_points[4]);
                return out;
            }
            case Type::HEXA_27: {
                std::vector<Parfait::Point<double>> out(27);
                out[0] = p1_points[0];
                out[1] = p1_points[1];
                out[2] = p1_points[2];
                out[3] = p1_points[3];
                out[4] = p1_points[4];
                out[5] = p1_points[5];
                out[6] = p1_points[5];
                out[7] = p1_points[5];

                out[8] = 0.5 * (p1_points[0] + p1_points[1]);
                out[9] = 0.5 * (p1_points[1] + p1_points[2]);
                out[10] = 0.5 * (p1_points[2] + p1_points[3]);
                out[11] = 0.5 * (p1_points[3] + p1_points[0]);

                out[12] = 0.5 * (p1_points[0] + p1_points[4]);
                out[13] = 0.5 * (p1_points[1] + p1_points[5]);
                out[14] = 0.5 * (p1_points[2] + p1_points[6]);
                out[15] = 0.5 * (p1_points[3] + p1_points[7]);

                out[16] = 0.5 * (p1_points[4] + p1_points[5]);
                out[17] = 0.5 * (p1_points[5] + p1_points[6]);
                out[18] = 0.5 * (p1_points[6] + p1_points[7]);
                out[19] = 0.5 * (p1_points[7] + p1_points[4]);

                out[20] = 0.25 * (p1_points[0] + p1_points[1] + p1_points[2] + p1_points[3]);

                out[21] = 0.25 * (p1_points[0] + p1_points[4] + p1_points[1] + p1_points[5]);
                out[22] = 0.25 * (p1_points[1] + p1_points[5] + p1_points[2] + p1_points[6]);
                out[23] = 0.25 * (p1_points[2] + p1_points[6] + p1_points[3] + p1_points[7]);
                out[24] = 0.25 * (p1_points[3] + p1_points[7] + p1_points[0] + p1_points[4]);

                out[25] = 0.25 * (p1_points[4] + p1_points[5] + p1_points[6] + p1_points[7]);

                out[26] = Parfait::Point<double>{0, 0, 0};
                for (int i = 0; i < 8; i++) {
                    out[26] += 0.125 * p1_points[i];
                }
                return out;
            }
            default:
                PARFAIT_THROW(
                    "Could not map types: " + MeshInterface::cellTypeString(Type::PYRA_5) + " to " +
                    MeshInterface::cellTypeString(inflated_type));
        }
    }

    std::vector<Parfait::Point<double>> elevate(
        const std::vector<Parfait::Point<double>>& p1_points,
        MeshInterface::CellType p1_type,
        MeshInterface::CellType inflated_type) {
        using Type = inf::MeshInterface::CellType;
        switch (p1_type) {
            case Type::BAR_2:
                return elevate_Bar_2(p1_points, inflated_type);
            case Type::TRI_3:
                return elevate_Tri_3(p1_points, inflated_type);
            case Type::QUAD_4:
                return elevate_QUAD_4(p1_points, inflated_type);
            case Type::TETRA_4:
                return elevate_Tet_4(p1_points, inflated_type);
            case Type::PYRA_5:
                return elevate_Pyra_5(p1_points, inflated_type);
            case Type::PENTA_6:
                return elevate_Penta_6(p1_points, inflated_type);
            case Type::HEXA_8:
                return elevate_Hexa_8(p1_points, inflated_type);
            default:
                PARFAIT_THROW("Could not map types: " + MeshInterface::cellTypeString(p1_type) +
                              " to " + MeshInterface::cellTypeString(inflated_type));
        }
    }
}  // P1ToP2
std::set<MeshInterface::CellType> extractAllCellTypes(MessagePasser mp, const TinfMesh& mesh) {
    auto types = Parfait::MapTools::keys(mesh.mesh.cell_tags);
    return mp.ParallelUnion(types);
}

class Elevator {
  public:
    Elevator(
        MessagePasser mp,
        const inf::TinfMesh& p1_mesh,
        std::map<MeshInterface::CellType, MeshInterface::CellType> type_map = inf::P1ToP2::getMap())
        : mp(mp), p1_mesh(p1_mesh), type_map(type_map) {
        domain = Parfait::ExtentBuilder::build(p1_mesh.mesh.points);
    }

    void copyP1CellData() {
        auto& p2_mesh = p2_mesh_ptr->mesh;
        for (auto p1_t : p1_types) {
            auto p2_t = type_map.at(p1_t);
            auto p2_t_length = inf::MeshInterface::cellTypeLength(p2_t);
            auto p1_t_length = inf::MeshInterface::cellTypeLength(p1_t);
            int count = p1_mesh.cellCount(p1_t);
            p2_mesh.cell_owner[p2_t] = p1_mesh.mesh.cell_owner.at(p1_t);
            p2_mesh.cell_tags[p2_t] = p1_mesh.mesh.cell_tags.at(p1_t);
            p2_mesh.global_cell_id[p2_t] = p1_mesh.mesh.global_cell_id.at(p1_t);
            p2_mesh.cells[p2_t].resize(count * p2_t_length, -1);
            for (int c = 0; c < count; c++) {
                for (int i = 0; i < p1_t_length; i++) {
                    p2_mesh.cells[p2_t][c * p2_t_length + i] =
                        p1_mesh.mesh.cells.at(p1_t)[c * p1_t_length + i];
                }
            }
        }
    }

    void copyP1NodeData() {
        auto& p2_mesh = p2_mesh_ptr->mesh;
        p2_mesh.node_owner = p1_mesh.mesh.node_owner;
        p2_mesh.global_node_id = p1_mesh.mesh.global_node_id;
        p2_mesh.points = p1_mesh.mesh.points;
    }

    void createOwnedAndGhostP2UniquePoints() {
        owned_unique_points = std::make_unique<Parfait::UniquePoints>(domain);
        ghost_unique_points = std::make_unique<Parfait::UniquePoints>(domain);

        auto get_set_owner = [&](const std::vector<int>& nodes) -> int {
            long lowest_global_id = std::numeric_limits<long>::max();
            int lowest_global_id_owner = -1;
            for (auto local_id : nodes) {
                auto global_id = p1_mesh.globalNodeId(local_id);
                if (global_id < lowest_global_id) {
                    lowest_global_id = global_id;
                    lowest_global_id_owner = p1_mesh.nodeOwner(local_id);
                }
            }
            return lowest_global_id_owner;
        };

        for (int c = 0; c < p1_mesh.cellCount(); c++) {
            inf::Cell cell(p1_mesh, c);
            auto cell_nodes = cell.nodes();
            // WARNING! The owner of the set of nodes of the cell
            // may not be the cell owner.
            // it is simply the owner of the lowest global id in the cell
            int cell_owner = get_set_owner(cell_nodes);
            auto p1_type = cell.type();
            auto p2_type = type_map.at(p1_type);
            if (inf::P1ToP2::hasMidCellPoint(p2_type)) {
                auto p = cell.averageCenter();
                if (cell_owner == mp.Rank()) {
                    owned_unique_points->getNodeId(p);
                } else {
                    int index = ghost_unique_points->getNodeId(p);
                    ghost_index_to_owner[index] = cell_owner;
                }
            }

            // handle faces
            for (int i = 0; i < cell.faceCount(); i++) {
                auto face_nodes = cell.faceNodes(i);
                auto face_points = cell.facePoints(i);
                bool is_quad_face = face_nodes.size() == 4;
                if (is_quad_face and inf::P1ToP2::hasMidQuadFacePoint(p2_type)) {
                    auto p = cell.faceAverageCenter(i);
                    int face_owner = get_set_owner(face_nodes);
                    if (face_owner == mp.Rank()) {
                        owned_unique_points->getNodeId(p);
                    } else {
                        auto index = ghost_unique_points->getNodeId(p);
                        ghost_index_to_owner[index] = face_owner;
                    }
                }

                // handle edges
                int num_face_nodes = face_nodes.size();
                for (int j = 0; j < num_face_nodes; j++) {
                    int k = (j + 1) % num_face_nodes;
                    auto p = face_points[j];
                    auto q = face_points[k];
                    auto mid_edge_location = (p + q) * 0.5;
                    int edge_owner = get_set_owner({face_nodes[j], face_nodes[k]});
                    if (edge_owner == mp.Rank()) {
                        owned_unique_points->getNodeId(mid_edge_location);
                    } else {
                        int index = ghost_unique_points->getNodeId(mid_edge_location);
                        ghost_index_to_owner[index] = edge_owner;
                    }
                }
            }
        }
    }

    void determineMyP2NodeGIDOffset() {
        long my_p2_node_count = owned_unique_points->points.size();
        auto everyone_node_counts = mp.Gather(my_p2_node_count);
        my_rank_new_node_gid_offset = inf::globalNodeCount(mp, p1_mesh);
        for (int r = 0; r < mp.Rank(); r++) {
            my_rank_new_node_gid_offset += everyone_node_counts.at(r);
        }
    }

    void determineMyGhostNodeGIDsByExchanging() {
        struct Request {
            Parfait::Point<double> xyz;
            int requester_ghost_index;
        };
        std::map<int, std::vector<Request>> requests;
        for (int index = 0; index < int(ghost_unique_points->points.size()); index++) {
            int owner = ghost_index_to_owner.at(index);
            Request r;
            r.requester_ghost_index = index;
            r.xyz = ghost_unique_points->points[index];
            requests[owner].push_back(r);
        }

        requests = mp.Exchange(requests);

        struct Reply {
            long global_id;
            int requester_ghost_index;
        };

        std::map<int, std::vector<Reply>> replies;

        for (auto& pair : requests) {
            auto requesting_rank = pair.first;
            for (auto request : pair.second) {
                auto my_p2_index = owned_unique_points->getNodeIdDontInsert(request.xyz);
                long gid = my_p2_index + my_rank_new_node_gid_offset;
                Reply reply;
                reply.global_id = gid;
                reply.requester_ghost_index = request.requester_ghost_index;
                replies[requesting_rank].push_back(reply);
            }
        }
        requests.clear();
        replies = mp.Exchange(replies);
        for (auto& pair : replies) {
            for (auto reply : pair.second) {
                auto gid = reply.global_id;
                auto ghost_index = reply.requester_ghost_index;
                ghost_index_to_global[ghost_index] = gid;
            }
        }
    }

    void addP2Nodes() {
        auto& p2_mesh = p2_mesh_ptr->mesh;
        int p1_node_count = p1_mesh.nodeCount();

        int p2_node_count = owned_unique_points->points.size();
        p2_mesh.points.resize(p2_node_count + p1_node_count);
        p2_mesh.node_owner.resize(p2_node_count + p1_node_count);
        p2_mesh.global_node_id.resize(p2_node_count + p1_node_count);

        for (auto& pair : p1_mesh.mesh.cell_tags) {
            auto p1_type = pair.first;
            int num_cells_in_type = pair.second.size();
            for (int cindex = 0; cindex < num_cells_in_type; cindex++) {
                inf::Cell cell(p1_mesh, p1_type, cindex);
                auto cell_points = cell.points();
                auto p2_type = type_map.at(p1_type);
                auto p2_points = inf::P1ToP2::elevate(cell_points, p1_type, p2_type);
                int p1_length = MeshInterface::cellTypeLength(p1_type);
                int p2_length = MeshInterface::cellTypeLength(p2_type);
                for (int i = p1_length; i < p2_length; i++) {
                    auto p = p2_points.at(i);
                    int p2_index = owned_unique_points->getNodeIdDontInsert(p);
                    auto local_node_id = p1_node_count + p2_index;
                    auto gid = p2_to_global.at(p2_index);
                    auto owner = p2_to_owner.at(p2_index);

                    p2_mesh.cells.at(p2_type).at(cindex * p2_length + i) = local_node_id;
                    PARFAIT_ASSERT_BOUNDS(p2_mesh.points, local_node_id, "points");
                    PARFAIT_ASSERT_BOUNDS(p2_mesh.global_node_id, local_node_id, "gid");
                    PARFAIT_ASSERT_BOUNDS(p2_mesh.node_owner, local_node_id, "owner");
                    p2_mesh.points.at(local_node_id) = p;
                    p2_mesh.global_node_id.at(local_node_id) = gid;
                    p2_mesh.node_owner.at(local_node_id) = owner;
                }
            }
        }
    }

    void mergeUniquePointsForEasyLookup() {
        int num_owned_p2 = owned_unique_points->points.size();
        for (int p2_index = 0; p2_index < num_owned_p2; p2_index++) {
            p2_to_global[p2_index] = p2_index + my_rank_new_node_gid_offset;
            p2_to_owner[p2_index] = mp.Rank();
        }
        int num_ghost_p2 = ghost_unique_points->points.size();
        for (int index = 0; index < num_ghost_p2; index++) {
            auto p = ghost_unique_points->points[index];
            int p2_index = owned_unique_points->getNodeId(p);
            p2_to_global[p2_index] = ghost_index_to_global[index];
            p2_to_owner[p2_index] = ghost_index_to_owner[index];
        }
    }

    auto elevate() {
        p2_mesh_ptr = std::make_shared<inf::TinfMesh>(inf::TinfMeshData(), mp.Rank());
        auto& p2_mesh = p2_mesh_ptr->mesh;

        p1_types = extractAllCellTypes(mp, p1_mesh);

        copyP1CellData();
        copyP1NodeData();
        p2_mesh.tag_names = p1_mesh.mesh.tag_names;

        createOwnedAndGhostP2UniquePoints();
        determineMyP2NodeGIDOffset();
        determineMyGhostNodeGIDsByExchanging();
        mergeUniquePointsForEasyLookup();

        addP2Nodes();

        p2_mesh_ptr->rebuild();
        return p2_mesh_ptr;
    }

  public:
    MessagePasser mp;
    const inf::TinfMesh& p1_mesh;
    Parfait::Extent<double> domain;
    std::map<MeshInterface::CellType, MeshInterface::CellType> type_map;
    std::set<MeshInterface::CellType> p1_types;
    std::shared_ptr<inf::TinfMesh> p2_mesh_ptr = nullptr;
    std::unique_ptr<Parfait::UniquePoints> owned_unique_points;
    std::unique_ptr<Parfait::UniquePoints> ghost_unique_points;
    std::map<int, int> ghost_index_to_owner;
    std::map<int, long> ghost_index_to_global;
    std::map<int, int> p2_to_owner;
    std::map<int, long> p2_to_global;
    long my_rank_new_node_gid_offset = -1;
};

std::shared_ptr<TinfMesh> elevateToP2(
    MessagePasser mp,
    const inf::TinfMesh& p1_mesh,
    std::map<MeshInterface::CellType, MeshInterface::CellType> type_map) {
    Elevator elevator(mp, p1_mesh, type_map);
    auto p2_mesh = elevator.elevate();

    for (int c = 0; c < p2_mesh->cellCount(); c++) {
        auto type = p2_mesh->cellType(c);
        std::vector<int> nodes = p2_mesh->cell(c);
        for (int n : nodes) {
            if (n < 0) {
                printf("Cell %d, type %s has a negative node: %s\n",
                       c,
                       MeshInterface::cellTypeString(type).c_str(),
                       Parfait::to_string(nodes).c_str());
            }
        }
    }
    return p2_mesh;
}

std::array<int, 2> findEdgeNeighboringNodesOfMidEdgeNode(const MeshInterface& mesh,
                                                         int cell_id,
                                                         int node_id) {
    auto cell_nodes = mesh.cell(cell_id);
    auto type = mesh.cellType(cell_id);

    if (type == inf::MeshInterface::CellType::TETRA_10) {
        if (cell_nodes[4] == node_id) {
            return {cell_nodes[0], cell_nodes[1]};
        }
        if (cell_nodes[5] == node_id) {
            return {cell_nodes[1], cell_nodes[2]};
        }
        if (cell_nodes[6] == node_id) {
            return {cell_nodes[2], cell_nodes[0]};
        }
        if (cell_nodes[7] == node_id) {
            return {cell_nodes[0], cell_nodes[3]};
        }
        if (cell_nodes[8] == node_id) {
            return {cell_nodes[1], cell_nodes[3]};
        }
        if (cell_nodes[9] == node_id) {
            return {cell_nodes[2], cell_nodes[3]};
        }
    }
    if (type == inf::MeshInterface::CellType::TRI_6) {
        if (cell_nodes[3] == node_id) {
            return {cell_nodes[0], cell_nodes[1]};
        }
        if (cell_nodes[4] == node_id) {
            return {cell_nodes[1], cell_nodes[2]};
        }
        if (cell_nodes[5] == node_id) {
            return {cell_nodes[2], cell_nodes[0]};
        }
    }
    return {-1, -1};
}

std::shared_ptr<inf::FieldInterface> cellToNodeReconstruction(
    const MeshInterface& mesh,
    const std::vector<Parfait::Point<double>>& cell_centroids,
    const std::vector<Parfait::Point<double>>& cell_gradients,
    const std::vector<Parfait::Point<double>>& node_gradients,
    const std::vector<std::vector<int>>& n2c,
    const FieldInterface& cell_field) {
    std::vector<double> node_field(mesh.nodeCount());

    for (int n = 0; n < mesh.nodeCount(); n++) {
        double a = 0.0;

        Parfait::Point<double> x = mesh.node(n);
        Parfait::Point<double> gradient;

        for (auto c : n2c[n]) {
            double field_value_at_cell = cell_field.getDouble(c);

            auto dx = x - cell_centroids[c];

            auto edge_nodes = findEdgeNeighboringNodesOfMidEdgeNode(mesh, c, n);

            if (edge_nodes[0] >= 0 and edge_nodes[1] >= 0) {
                auto edge_averaged_gradient =
                    0.5 * (node_gradients[edge_nodes[0]] + node_gradients[edge_nodes[1]]);

                gradient = 0.5 * (cell_gradients[c] + edge_averaged_gradient);
            } else {
                PARFAIT_ASSERT_BOUNDS(cell_gradients, c, "cell grad");
                PARFAIT_ASSERT_BOUNDS(node_gradients, n, "node grad");
                gradient = 0.5 * (cell_gradients[c] + node_gradients[n]);
            }
            gradient = cell_gradients[c];

            double a_reconstructed = field_value_at_cell + gradient.dot(dx);

            a += a_reconstructed;
        }
        node_field[n] = a / (double)n2c[n].size();
    }

    return std::make_shared<inf::VectorFieldAdapter>(
        cell_field.name() + "-nodal-reconstructed", inf::FieldAttributes::Node(), node_field);
}

std::shared_ptr<inf::FieldInterface> interpolateCellFieldToNodes(
    const MeshInterface& p1_mesh,
    const MeshInterface& p2_mesh,
    const CellToNodeGradientCalculator& cell_to_node_grad_calculator,
    const std::vector<std::vector<int>>& node_to_cell_p2,
    const inf::FieldInterface& cell_field) {
    auto node_gradients = cell_to_node_grad_calculator.calcGrad(cell_field);

    int num_cells = p1_mesh.cellCount();
    std::vector<Parfait::Point<double>> cell_averaged_gradient(num_cells);
    for (int c = 0; c < num_cells; c++) {
        auto cell_nodes = p1_mesh.cell(c);
        for (int n : cell_nodes) {
            cell_averaged_gradient[c] += node_gradients[n];
        }
        cell_averaged_gradient[c] *= 1.0 / (double)cell_nodes.size();
    }

    std::vector<Parfait::Point<double>> cell_centroids(num_cells);
    for (int c = 0; c < p2_mesh.cellCount(); c++) {
        inf::Cell cell(p1_mesh, c);
        cell_centroids[c] = cell.averageCenter();
    }

    return cellToNodeReconstruction(p2_mesh,
                                    cell_centroids,
                                    cell_averaged_gradient,
                                    node_gradients,
                                    node_to_cell_p2,
                                    cell_field);
}
}