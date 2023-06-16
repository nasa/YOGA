#include "MeshSanityChecker.h"
#include <parfait/FileTools.h>
#include <parfait/Dictionary.h>
#include <parfait/JsonParser.h>
#include <parfait/PointWriter.h>
#include "SliverCellDetection.h"
#include <parfait/ToString.h>

using namespace inf;

void throwIfDuplicatesFound(const MeshInterface& mesh) {
    std::set<long> gids;
    for (int i = 0; i < mesh.nodeCount(); i++) gids.insert(mesh.globalNodeId(i));
    if (int(gids.size()) != mesh.nodeCount()) throw std::logic_error(__FUNCTION__);
}
void throwIFAnyoneErrors(MessagePasser mp, std::string message = "") {
    int error = 0;
    error = mp.ParallelMax(error);
    if (error) {
        throw std::logic_error("");
    }
}
bool isTriangleAlignedWithDirection(const Parfait::Point<double>& direction,
                                    const Parfait::Point<double>& a,
                                    const Parfait::Point<double>& b,
                                    const Parfait::Point<double>& c) {
    auto face_area = Parfait::Metrics::computeTriangleArea(a, b, c);
    face_area.normalize();
    return (Parfait::Point<double>::dot(direction, face_area) > 0.0);
}

bool allSuccess(const MeshSanityChecker::ResultLog& results) {
    for (auto& pair : results) {
        if (pair.second == false) {
            return false;
        }
    }
    return true;
}

std::vector<double> MeshSanityChecker::minimumCellCentroidDistanceInNodeStencil(
    const MeshInterface& mesh, const std::vector<std::vector<int>>& n2c) {
    int num_nodes = mesh.nodeCount();
    std::vector<double> min_centroid_distance(mesh.nodeCount(), std::numeric_limits<double>::max());
    for (int n = 0; n < num_nodes; n++) {
        for (auto c1 : n2c[n]) {
            auto cell_1 = inf::Cell(mesh, c1);
            for (auto c2 : n2c[n]) {
                if (c1 == c2) continue;
                auto cell_2 = inf::Cell(mesh, c2);
                auto d = cell_1.averageCenter() - cell_2.averageCenter();
                min_centroid_distance[n] = std::min(min_centroid_distance[n], d.magnitude());
            }
        }
    }
    return min_centroid_distance;
}

std::vector<double> MeshSanityChecker::minimumCellHiroCentroidDistanceInNodeStencil(
    const MeshInterface& mesh, const std::vector<std::vector<int>>& n2c, double hiro_p) {
    int num_nodes = mesh.nodeCount();
    std::vector<double> min_centroid_distance(mesh.nodeCount(), std::numeric_limits<double>::max());
    for (int n = 0; n < num_nodes; n++) {
        for (auto c1 : n2c[n]) {
            auto cell_1 = inf::Cell(mesh, c1);
            for (auto c2 : n2c[n]) {
                if (c1 == c2) continue;
                auto cell_2 = inf::Cell(mesh, c2);
                auto d = inf::Hiro::calcCentroid(cell_1, hiro_p) -
                         inf::Hiro::calcCentroid(cell_2, hiro_p);
                min_centroid_distance[n] = std::min(min_centroid_distance[n], d.magnitude());
            }
        }
    }
    return min_centroid_distance;
}

bool doAllOwnedCellsHaveNeighbors(const MeshInterface& mesh,
                                  const std::vector<std::vector<int>>& face_neighbors) {
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        if (mesh.ownedCell(cell_id)) {
            auto& neighbors = face_neighbors[cell_id];
            for (auto& n : neighbors) {
                if (n < 0) {
                    auto type = mesh.cellType(cell_id);
                    printf(
                        "Cell global cell %lu of type %s is owned, but doesn't have a neighbor.\n",
                        mesh.globalCellId(cell_id),
                        inf::MeshInterface::cellTypeString(type).c_str());
                    return false;
                }
            }
        }
    }
    return true;
}
void checkIAgreeTheseCellsAreOwnedByRank(const MeshInterface& mesh,
                                         const std::map<long, int>& global_to_local_cell,
                                         const std::vector<long>& global_cell_ids_owned_by_rank,
                                         int rank) {
    for (auto gid : global_cell_ids_owned_by_rank) {
        int local_cell_id = -1;
        if (global_to_local_cell.count(gid) > 0) local_cell_id = global_to_local_cell.at(gid);
        if (local_cell_id >= 0) {
            if (mesh.cellOwner(local_cell_id) != rank) {
                std::string error = "Rank " + std::to_string(rank) + " claims they own cell " +
                                    std::to_string(gid) + " but I think rank " +
                                    std::to_string(mesh.cellOwner(local_cell_id)) + " owns it.";
                throw std::logic_error(error);
            }
        }
    }
}
void checkIAgreeTheseNodesAreOwnedByRank(MessagePasser mp,
                                         const MeshInterface& mesh,
                                         const std::map<long, int>& global_to_local,
                                         const std::vector<long>& global_node_ids_owned_by_rank,
                                         int rank) {
    for (auto gid : global_node_ids_owned_by_rank) {
        int local_id = -1;
        if (global_to_local.count(gid) > 0) local_id = global_to_local.at(gid);
        if (local_id >= 0) {
            if (mesh.nodeOwner(local_id) != rank) {
                std::string error = "Rank " + std::to_string(rank) + " claims they own node " +
                                    std::to_string(gid) + " but rank " + std::to_string(mp.Rank()) +
                                    " thinks rank " + std::to_string(mesh.nodeOwner(local_id)) +
                                    " owns it.";
                throw std::logic_error(error);
            }
        }
    }
}
std::map<long, int> buildGlobalToLocalCell(const MeshInterface& mesh) {
    std::map<long, int> global_to_local;
    for (int local = 0; local < mesh.cellCount(); local++) {
        global_to_local[mesh.globalCellId(local)] = local;
    }
    return global_to_local;
}
std::map<long, int> buildGlobalToLocalNode(const MeshInterface& mesh) {
    std::map<long, int> global_to_local;
    for (int local = 0; local < mesh.nodeCount(); local++) {
        global_to_local[mesh.globalNodeId(local)] = local;
    }
    return global_to_local;
}
void checkCellsOwnedUniquely(const MeshInterface& mesh, MessagePasser mp) {
    std::map<long, int> global_to_local_cell = buildGlobalToLocalCell(mesh);
    for (int r = 0; r < mp.NumberOfProcesses(); r++) {
        std::vector<long> owned_by_rank;
        if (mp.Rank() == r) {
            for (int c = 0; c < mesh.cellCount(); c++) {
                if (mesh.cellOwner(c) == mp.Rank()) owned_by_rank.push_back(mesh.globalCellId(c));
            }
        }
        mp.Broadcast(owned_by_rank, r);
        if (mp.Rank() != r) {
            checkIAgreeTheseCellsAreOwnedByRank(mesh, global_to_local_cell, owned_by_rank, r);
        }
    }
}

void throwAnError(MessagePasser mp) { mp.ParallelMax(1); }

void checkNodesOwnedUniquely(const MeshInterface& mesh, MessagePasser mp) {
    try {
        std::map<long, int> global_to_local_node = buildGlobalToLocalNode(mesh);
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            std::vector<long> owned_by_rank;
            if (mp.Rank() == r) {
                for (int n = 0; n < mesh.nodeCount(); n++) {
                    if (mesh.nodeOwner(n) == mp.Rank())
                        owned_by_rank.push_back(mesh.globalNodeId(n));
                }
            }
            throwIFAnyoneErrors(mp);
            mp.Broadcast(owned_by_rank, r);
            if (mp.Rank() != r) {
                checkIAgreeTheseNodesAreOwnedByRank(
                    mp, mesh, global_to_local_node, owned_by_rank, r);
            }
        }
    } catch (std::exception& e) {
        PARFAIT_WARNING(e.what());
        throwAnError(mp);
        throw e;
    }
}

bool checkAllNodesAreResident(const MeshInterface& mesh) {
    int num_nodes_on_part = mesh.nodeCount();
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        inf::Cell cell(mesh, cell_id);
        auto nodes = cell.nodes();
        for (auto n : nodes) {
            if (n < 0 or n >= num_nodes_on_part) {
                PARFAIT_WARNING("Mesh contains a cell with node but no xyz.  " + std::to_string(n) +
                                ", nodeCount() = " + std::to_string(mesh.nodeCount()));
                return false;
            }
        }
    }
    return true;
}

bool checkOwnedNodesComeFirst(const MeshInterface& mesh) {
    bool found_ghost = false;
    for (int node_id = 0; node_id < mesh.nodeCount(); node_id++) {
        if (found_ghost and mesh.ownedNode(node_id)) {
            return false;
        }
        if (not mesh.ownedNode(node_id)) found_ghost = true;
    }
    return true;
}

bool checkAllNodesExistInACell(const MeshInterface& mesh) {
    bool found_no_island_nodes = true;

    auto n2c = inf::NodeToCell::build(mesh);
    std::vector<Parfait::Point<double>> island_points;
    for (int node_id = 0; node_id < mesh.nodeCount(); node_id++) {
        if (mesh.ownedNode(node_id) and n2c[node_id].size() == 0) {
            found_no_island_nodes = false;
            Parfait::Point<double> p = mesh.node(node_id);
            printf("Node %d is an island: %s\n", node_id, p.to_string().c_str());
            island_points.push_back(p);
            // break;
        }
    }

    if (not island_points.empty()) {
        auto mp = MessagePasser(MPI_COMM_WORLD);
        Parfait::PointWriter::write("island-points." + std::to_string(mp.Rank()), island_points);
    }

    return found_no_island_nodes;
}

inline std::vector<std::set<long>> convertToGlobalCellId(const inf::MeshInterface& mesh,
                                                         const std::vector<std::vector<int>>& n2c) {
    std::vector<std::set<long>> n2c_global(n2c.size());
    for (size_t i = 0; i < n2c.size(); i++) {
        for (int local : n2c[i]) {
            long global = mesh.globalCellId(local);
            n2c_global[i].insert(global);
        }
    }

    return n2c_global;
}

inline void checkOwnedNodesHaveFirstLayerStencilSupport(MessagePasser mp,
                                                        const MeshInterface& mesh) {
    auto g2l_node = inf::GlobalToLocal::buildNode(mesh);
    auto n2c_local = inf::NodeToCell::build(mesh);
    auto n2c = convertToGlobalCellId(mesh, n2c_local);
    struct Pair {
        long node;
        long cell;
    };
    std::map<int, std::vector<Pair>> node_cell_pairs_to_ranks;
    for (int n = 0; n < mesh.nodeCount(); n++) {
        int owner = mesh.nodeOwner(n);
        if (owner != mesh.partitionId()) {
            for (auto c : n2c[n])
                node_cell_pairs_to_ranks[owner].push_back({mesh.globalNodeId(n), c});
        }
    }

    auto incoming = mp.Exchange(node_cell_pairs_to_ranks);
    node_cell_pairs_to_ranks.clear();
    for (auto rank_and_nodecell_pairs : incoming) {
        auto rank = rank_and_nodecell_pairs.first;
        auto& nodecell_pairs = rank_and_nodecell_pairs.second;
        for (auto pair : nodecell_pairs) {
            auto global_node = pair.node;
            auto global_cell = pair.cell;
            if (g2l_node.count(global_node) == 0) {
                PARFAIT_THROW("Rank " + std::to_string(rank) + " thinks node: " +
                              std::to_string(global_node) + " is owned by rank " +
                              std::to_string(mp.Rank()) + " but it doesn't have the node resident.")
            }
            int local_node = g2l_node.at(global_node);
            if (n2c[local_node].count(global_cell) == 0) {
                PARFAIT_THROW("Rank " + std::to_string(mp.Rank()) + " owns node " +
                              std::to_string(global_node) + " but is missing adjacent cell " +
                              std::to_string(global_cell) + " resident on rank " +
                              std::to_string(rank));
            }
        }
    }
}

MeshSanityChecker::ResultLog MeshSanityChecker::checkAll(const MeshInterface& mesh,
                                                         MessagePasser mp) {
    ResultLog result_log;
    try {
        result_log = checkAllSerial(mp, mesh);
        bool good = allSuccess(result_log);
        if (not good) {
            mp_rootprint("Serial checks failed.  Skipping parallel tests");
            return result_log;
        }

        mp_rootprint("Check that all cells are owned uniquely");
        checkCellsOwnedUniquely(mesh, mp);
        throwIFAnyoneErrors(mp);
        mp_rootprint("... done!\n");

        mp_rootprint("Check that all nodes are owned uniquely");
        checkNodesOwnedUniquely(mesh, mp);
        throwIFAnyoneErrors(mp);
        mp_rootprint("... done!\n");

        mp_rootprint("Check owned nodes have full cell support");
        checkOwnedNodesHaveFirstLayerStencilSupport(mp, mesh);
        throwIFAnyoneErrors(mp);
        mp_rootprint("... done!\n");
    } catch (const std::exception& e) {
        if (not std::string(e.what()).empty()) {
            PARFAIT_WARNING("Sanity checker failed with: " + std::string(e.what()));
        }
        return result_log;
    }
    return result_log;
}

std::set<long> getFaceNodeNeighborsGlobal(const MeshInterface& mesh,
                                          int cell_id,
                                          int face_number,
                                          const std::vector<std::vector<int>>& n2c) {
    std::set<long> node_neighbors;
    inf::Cell cell(mesh, cell_id);
    for (auto n : cell.faceNodes(face_number)) {
        auto n_neighbors = n2c[n];
        for (auto c : n_neighbors) node_neighbors.insert(mesh.globalCellId(c));
    }
    return node_neighbors;
}

std::set<int> getAllNodeNeighbors(const MeshInterface& mesh,
                                  int cell_id,
                                  const std::vector<std::vector<int>>& n2c) {
    std::set<int> node_neighbors;
    std::vector<int> cell;
    mesh.cell(cell_id, cell);
    for (auto n : cell) {
        auto n_neighbors = n2c[n];
        for (auto c : n_neighbors) node_neighbors.insert(c);
    }
    return node_neighbors;
}

HangingEdgeNeighborhood MeshSanityChecker::checkForTwoTriFacesOnQuad(
    const MeshInterface& mesh, int cell_id, const std::vector<std::vector<int>>& n2c) {
    HangingEdgeNeighborhood neighborhood;
    auto cell = inf::Cell(mesh, cell_id);
    for (int face_number = 0; face_number < cell.faceCount(); face_number++) {
        auto face_nodes = cell.faceNodes(face_number);
        auto candidate_node_neighbors = getAllNodeNeighbors(mesh, cell_id, n2c);
        candidate_node_neighbors.erase(cell_id);
        auto neighbor =
            inf::FaceNeighbors::findFaceNeighbor(mesh, candidate_node_neighbors, face_nodes);
        if (neighbor < 0) {
            if (face_nodes.size() == 4) {
                neighborhood.setQuadCell(cell_id);
                // --- one diagonal
                std::vector<int> tri1(3);
                std::vector<int> tri2(3);
                tri1[0] = face_nodes[0];
                tri1[1] = face_nodes[1];
                tri1[2] = face_nodes[2];

                tri2[0] = face_nodes[2];
                tri2[1] = face_nodes[3];
                tri2[2] = face_nodes[0];

                // --- other diagonal
                std::vector<int> tri3(3);
                std::vector<int> tri4(3);
                tri3[0] = face_nodes[1];
                tri3[1] = face_nodes[2];
                tri3[2] = face_nodes[3];

                tri4[0] = face_nodes[3];
                tri4[1] = face_nodes[0];
                tri4[2] = face_nodes[1];

                auto tri_neighbor_1 =
                    inf::FaceNeighbors::findFaceNeighbor(mesh, candidate_node_neighbors, tri1);
                auto tri_neighbor_2 =
                    inf::FaceNeighbors::findFaceNeighbor(mesh, candidate_node_neighbors, tri2);
                auto tri_neighbor_3 =
                    inf::FaceNeighbors::findFaceNeighbor(mesh, candidate_node_neighbors, tri3);
                auto tri_neighbor_4 =
                    inf::FaceNeighbors::findFaceNeighbor(mesh, candidate_node_neighbors, tri4);

                //                auto build_message = [&](int tri_neighbor, const std::vector<int>&
                //                tri_nodes) {
                //                    std::string message =
                //                        "Found hanging edge adjacent to global cell " +
                //                        std::to_string(mesh.globalCellId(cell_id));
                //                    message += " type " +
                //                    mesh.cellTypeString(mesh.cellType(cell_id)) + " "; message +=
                //                    "face nodes: " + Parfait::to_string(face_nodes) + "\n";
                //                    message += "matching tri face of global cell " +
                //                    std::to_string(mesh.globalCellId(tri_neighbor)); message += "
                //                    type " + mesh.cellTypeString(mesh.cellType(tri_neighbor)) + "
                //                    "; message += "face nodes: "
                //                    + Parfait::to_string(tri_nodes) + "\n"; return message;
                //                };
                if (tri_neighbor_1 >= 0) {
                    neighborhood.addTriCell(tri_neighbor_1);
                    //                    printf("%s", build_message(tri_neighbor_1, tri1).c_str());
                }
                if (tri_neighbor_2 >= 0) {
                    neighborhood.addTriCell(tri_neighbor_2);
                    //                    printf("%s", build_message(tri_neighbor_2, tri2).c_str());
                }
                if (tri_neighbor_3 >= 0) {
                    neighborhood.addTriCell(tri_neighbor_3);
                    //                    printf("%s", build_message(tri_neighbor_3, tri3).c_str());
                }
                if (tri_neighbor_4 >= 0) {
                    neighborhood.addTriCell(tri_neighbor_4);
                    //                    printf("%s", build_message(tri_neighbor_4, tri4).c_str());
                }
            }
        }
    }
    return neighborhood;
}

std::vector<HangingEdgeNeighborhood> MeshSanityChecker::findHangingEdges(
    const MeshInterface& mesh,
    const std::vector<std::vector<int>>& n2c,
    std::vector<std::vector<int>>& face_neighbors) {
    std::vector<HangingEdgeNeighborhood> neighborhoods;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        if (mesh.ownedCell(cell_id)) {
            for (unsigned int face_number = 0; face_number < face_neighbors.at(cell_id).size();
                 face_number++) {
                auto neighbor = face_neighbors.at(cell_id).at(face_number);
                if (neighbor < 0) {
                    auto neighborhood =
                        MeshSanityChecker::checkForTwoTriFacesOnQuad(mesh, cell_id, n2c);
                    if (neighborhood.isHangingEdge()) {
                        neighborhoods.push_back(neighborhood);
                    }
                }
            }
        }
    }
    return neighborhoods;
}

std::vector<HangingEdgeNeighborhood> MeshSanityChecker::findHangingEdges(
    const MeshInterface& mesh) {
    auto n2c = inf::NodeToCell::build(mesh);
    auto face_neighbors = inf::FaceNeighbors::build(mesh, n2c);
    return findHangingEdges(mesh, n2c, face_neighbors);
}

bool MeshSanityChecker::checkDuplicatedFaceNeighbors(
    const MeshInterface& mesh,
    const std::vector<std::vector<int>>& n2c,
    const std::vector<std::vector<int>>& face_neighbors) {
    bool good = true;
    for (int c = 0; c < mesh.cellCount(); c++) {
        std::set<int> found_neighbors;
        for (auto neighbor : face_neighbors[c]) {
            if (neighbor < 0) continue;
            if (found_neighbors.count(neighbor)) {
                if (good) printf("\n");
                good = false;
                printf(
                    "Cell %ld type %s face is connected to neighbor %ld type %s multiple times\n",
                    mesh.globalCellId(c),
                    inf::MeshInterface::cellTypeString(mesh.cellType(c)).c_str(),
                    mesh.globalCellId(neighbor),
                    inf::MeshInterface::cellTypeString(mesh.cellType(neighbor)).c_str());
            }
            found_neighbors.insert(neighbor);
        }
    }
    return good;
}

bool MeshSanityChecker::checkOwnedCellsHaveFaceNeighbors(
    const MeshInterface& mesh,
    const std::vector<std::vector<int>>& n2c,
    const std::vector<std::vector<int>>& face_neighbors) {
    bool good = true;
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        if (mesh.ownedCell(cell_id)) {
            for (unsigned int face_number = 0; face_number < face_neighbors.at(cell_id).size();
                 face_number++) {
                auto neighbor = face_neighbors.at(cell_id).at(face_number);
                if (neighbor < 0) {
                    good = false;

                    auto cell = inf::Cell(mesh, cell_id);
                    auto center = cell.averageCenter();
                    auto node_neighbors =
                        getFaceNodeNeighborsGlobal(mesh, cell_id, face_number, n2c);

                    std::string message = "Found owned cell missing face neighbor on face " +
                                          std::to_string(face_number) + "\n";
                    message +=
                        "Type: " + inf::MeshInterface::cellTypeString(mesh.cellType(cell_id)) + " ";
                    message += "Location : " + center.to_string() + "\n";

                    std::string verbose;
                    verbose += "LID: " + std::to_string(cell_id) + ", ";
                    verbose += "GID: " + std::to_string(mesh.globalCellId(cell_id)) + "\n";
                    verbose += "cell nodes: " + Parfait::to_string(cell.nodes()) + "\n";
                    verbose +=
                        "face nodes: " + Parfait::to_string(cell.faceNodes(face_number)) + "\n";
                    verbose += "extent: " + cell.extent().to_string() + "\n";
                    verbose += "Found face neighbors: ";
                    verbose += Parfait::to_string(face_neighbors.at(cell_id)) + "\n";
                    verbose += "Node neighbors: " + Parfait::to_string(node_neighbors) + "\n";

                    auto hanging_edge =
                        MeshSanityChecker::checkForTwoTriFacesOnQuad(mesh, cell_id, n2c);
                    std::string filename = "no-neighbor";
                    if (hanging_edge.isHangingEdge()) {
                        message += "Found hanging edge, check previous output \n";
                        filename += "hanging-edge";
                        printf("%s\n", message.c_str());
                    } else {
                        message += "No hanging edge found.\n";
                    }

                    cell.visualize(filename + std::to_string(mesh.globalCellId(cell_id)));

                    message += verbose;
                    message = Parfait::to_string(face_neighbors.at(cell_id)) + " ";
                    message += Parfait::to_string(node_neighbors) + "\n";

                    Parfait::FileTools::appendToFile("bad_stencils.txt", message);
                }
            }
        }
    }
    return good;
}

bool MeshSanityChecker::isMeshValid(const MeshInterface& mesh, MessagePasser mp) {
    auto results = checkAll(mesh, mp);
    return allSuccess(results);
}

void checkThatDataExistsForEachNode(const MeshInterface& mesh) {
    std::array<double, 3> p;
    for (int i = 0; i < mesh.nodeCount(); i++) {
        mesh.nodeCoordinate(i, p.data());
        int owner = mesh.nodeOwner(i);
        long global_id = mesh.globalNodeId(i);
        if (owner < 0) throw std::logic_error("node owner is negative!");
        if (global_id < 0) throw std::logic_error("node global ID is negative!");
    }
}

bool MeshSanityChecker::checkGlobalNodeIds(const MeshInterface& mesh) {
    try {
        throwIfDuplicatesFound(mesh);
    } catch (std::exception& e) {
        return false;
    }
    return true;
}

bool checkForSliverCells(const MeshInterface& mesh, double tolerance = 3e-10) {
    if (Parfait::FileTools::doesFileExist(".tinf-backdoor")) {
        auto settings_string = Parfait::FileTools::loadFileToString(".tinf-backdoor");
        PARFAIT_WARNING(".tinf-backdoor found!\n");
        auto json = Parfait::JsonParser::parse(settings_string);
        if (json.has("sliver cell tolerance")) {
            tolerance = json.at("sliver cell tolerance").asDouble();
        }
    }
    bool good = true;
    double min_volume = std::numeric_limits<double>::max();
    double min_area = std::numeric_limits<double>::max();
    for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
        auto cell = inf::Cell(mesh, cell_id);
        auto type = cell.type();
        if (inf::MeshInterface::is2DCellType(type)) {
            auto area = cell.faceAreaNormal(0);
            if (area.magnitude() < tolerance) {
                good = false;
                printf("%s near %s is sliver %e\n",
                       inf::MeshInterface::cellTypeString(type).c_str(),
                       cell.averageCenter().to_string().c_str(),
                       area.magnitude());
                std::string message = Parfait::to_string(mesh.globalCellId(cell_id)) + "\n";
                Parfait::FileTools::appendToFile("bad_stencils.txt", message);
            }
            min_area = std::min(area.magnitude(), min_area);
        }
        if (inf::MeshInterface::is3DCellType(type)) {
            auto volume = cell.volume();
            if (volume < tolerance) {
                good = false;
                printf("%s near %s is sliver %e\n",
                       inf::MeshInterface::cellTypeString(type).c_str(),
                       cell.averageCenter().to_string().c_str(),
                       volume);
                std::string message = Parfait::to_string(mesh.globalCellId(cell_id)) + "\n";
                Parfait::FileTools::appendToFile("bad_stencils.txt", message);
            }
            min_volume = std::min(volume, min_volume);
        }
    }

    return good;
}

bool MeshSanityChecker::checkGlobalCellIds(const MeshInterface& mesh) {
    for (int i = 0; i < mesh.cellCount(); i++) {
        long gid = mesh.globalCellId(i);
        if (gid < 0) {
            PARFAIT_WARNING("global id is negative ");
            return false;
        }
    }
    return true;
}

bool MeshSanityChecker::checkCellOwners(const MeshInterface& mesh) {
    for (int i = 0; i < mesh.cellCount(); i++) {
        int owner = mesh.cellOwner(i);
        if (owner < 0) {
            PARFAIT_WARNING("Cell owner is negative");
            return false;
        }
    }
    return true;
}

bool MeshSanityChecker::isQuadAllignedConsistently(
    const std::vector<Parfait::Point<double>>& face_points) {
    auto tri1 = Parfait::Point<double>::normalize(
        Parfait::Metrics::computeTriangleArea(face_points[0], face_points[1], face_points[2]));
    auto tri2 = Parfait::Point<double>::normalize(
        Parfait::Metrics::computeTriangleArea(face_points[2], face_points[3], face_points[0]));
    return Parfait::Point<double>::dot(tri1, tri2) > 0.0;
}
bool isFaceValid(const Parfait::Point<double>& pointing_out,
                 const std::vector<Parfait::Point<double>>& face_points) {
    if (face_points.size() == 3) {
        if (not isTriangleAlignedWithDirection(
                pointing_out, face_points[0], face_points[1], face_points[2]))
            return false;
    } else if (face_points.size() == 4) {
        if (not isTriangleAlignedWithDirection(
                pointing_out, face_points[2], face_points[3], face_points[0]))
            return false;
        if (not MeshSanityChecker::isQuadAllignedConsistently(face_points)) return false;
    }
    return true;
}
bool MeshSanityChecker::areCellFacesValid(const Cell& cell) {
    auto center = cell.averageCenter();
    for (int face_number = 0; face_number < cell.faceCount(); face_number++) {
        auto face_center = cell.faceAverageCenter(face_number);
        auto pointing_out = Parfait::Point<double>::normalize(face_center - center);
        auto face_points = cell.facePoints(face_number);
        if (not isFaceValid(pointing_out, face_points)) return false;
    }
    return true;
}
bool MeshSanityChecker::isCellClosed(const inf::Cell& cell) {
    Parfait::Point<double> surface_integral = {0, 0, 0};
    for (int f = 0; f < cell.faceCount(); f++) {
        auto face_normal = cell.faceAreaNormal(f);
        surface_integral += face_normal;
    }
    return surface_integral.magnitude() < 1e-10;
}
bool MeshSanityChecker::checkVolumeCellsAreClosed(const inf::MeshInterface& mesh) {
    for (int i = 0; i < mesh.cellCount(); i++) {
        Cell cell(mesh, i);
        if (inf::MeshInterface::is3DCellType(cell.type())) {
            if (not MeshSanityChecker::isCellClosed(cell)) {
                PARFAIT_WARNING("Cell is not closed");
                return false;
            }
        }
    }
    return true;
}
bool MeshSanityChecker::checkFaceWindings(const inf::MeshInterface& mesh) {
    for (int i = 0; i < mesh.cellCount(); i++) {
        Cell cell(mesh, i);
        if (not inf::MeshInterface::is3DCellType(cell.type())) continue;
        if (not MeshSanityChecker::areCellFacesValid(cell)) {
            std::string error_message =
                "Found a " + inf::MeshInterface::cellTypeString(cell.type());
            error_message += " with a face normal pointing towards the cell's average centroid.\n";
            error_message += "Location " + cell.averageCenter().to_string() + "\n";
            error_message +=
                "This could indicate the cell is highly skewed, or that the cell has an invalid "
                "winding.\n";
            cell.visualize("twisted-cell-" + std::to_string(mesh.globalCellId(i)));
            printf("%s", error_message.c_str());
            return false;
        }
    }
    return true;
}

bool MeshSanityChecker::checkPositiveVolumes(const inf::MeshInterface& mesh) {
    for (int i = 0; i < mesh.cellCount(); i++) {
        Cell cell(mesh, i);
        if (not inf::MeshInterface::is3DCellType(cell.type())) continue;
        auto volume = cell.volume();
        if (volume < 0.0) {
            std::string error_message =
                "Found a " + inf::MeshInterface::cellTypeString(cell.type());
            error_message += " with a negative volume\n";
            error_message += "Location " + cell.averageCenter().to_string() + "\n";
            cell.visualize("negative-volume-" + std::to_string(mesh.globalCellId(i)));
            printf("%s", error_message.c_str());
            return false;
        }
    }
    return true;
}

MeshSanityChecker::ResultLog MeshSanityChecker::checkAllSerial(MessagePasser mp,
                                                               const MeshInterface& mesh) {
    MeshSanityChecker::ResultLog result_log;
    Tracer::begin("MeshSanityChecker::checkAllSerial");
    bool good = true;
    mp_rootprint("Mesh Sanity Checker\n");

    mp_rootprint("Check that all nodes in cells have resident xyz");
    good = checkAllNodesAreResident(mesh);
    good = mp.ParallelAnd(good);
    mp_rootprint(".... %s!\n", Parfait::to_string(good).c_str());
    result_log["AllNodesAreResident"] = good;

    mp_rootprint("Check that global Cell Ids are valid");
    good = checkGlobalCellIds(mesh);
    good = mp.ParallelAnd(good);
    mp_rootprint(".... %s!\n", Parfait::to_string(good).c_str());
    result_log["GlobalCellIds"] = good;

    mp_rootprint("Check that Cell Owners are valid");
    good = checkCellOwners(mesh);
    good = mp.ParallelAnd(good);
    mp_rootprint(".... %s!\n", Parfait::to_string(good).c_str());
    result_log["CellOwners"] = good;

    mp_rootprint("Check that volume cells have positive volume");
    good = checkPositiveVolumes(mesh);
    good = mp.ParallelAnd(good);
    result_log["PositiveVolumes"] = good;
    std::string message;
    if (good) {
        message = "No problems detected";
    } else {
        message = "Negative volume elements detected";
    }
    mp_rootprint(".... %s!\n", message.c_str());

    //    mp_rootprint("Check that cells have valid face windings");
    //    good = checkFaceWindings(mesh);
    //    good = mp.ParallelAnd(good);
    //    result_log["FaceWindings"] = good;
    //    if (good) {
    //        message = "No problems detected";
    //    } else {
    //        message = "Poor quality elements detected";
    //    }
    //    mp_rootprint(".... %s!\n", message.c_str());
    //
    mp_rootprint("Check volume cells are closed");
    good = checkVolumeCellsAreClosed(mesh);
    good = mp.ParallelAnd(good);
    result_log["VolumeCellsAreClosed"] = good;
    if (good) {
        message = "No problems detected";
    } else {
        message = "Poor quality elements detected";
    }
    mp_rootprint(".... %s!\n", message.c_str());

    mp_rootprint("Check owned cells have face neighbors");
    auto n2c = inf::NodeToCell::build(mesh);
    auto face_neighbors = inf::FaceNeighbors::build(mesh, n2c);
    good = checkOwnedCellsHaveFaceNeighbors(mesh, n2c, face_neighbors);
    good = mp.ParallelAnd(good);
    result_log["OwnedCellsHaveFaceNeighbors"] = good;
    mp_rootprint(".... %s!\n", Parfait::to_string(good).c_str());

    mp_rootprint("Check for cells with duplicated face neighbors");
    good = checkDuplicatedFaceNeighbors(mesh, n2c, face_neighbors);
    good = mp.ParallelAnd(good);
    result_log["DuplicateFaceNeighbors"] = good;
    if (good) {
        mp_rootprint(".... No problems detected\n");
    } else {
        mp_rootprint(".... Duplicate face neighbors found\n");
    }

    if (Parfait::FileTools::doesFileExist(mp, "fun3d.nml")) {
        mp_rootprint("fun3d.nml found.  Enabling Fun3D specific checks\n");

        mp_rootprint("Check owned nodes come first");
        good &= checkOwnedNodesComeFirst(mesh);
        good = mp.ParallelAnd(good);
        mp_rootprint(".... %s!\n", Parfait::to_string(good).c_str());
        if (not good) {
            mp_rootprint("  Some Fun3D components require this.\n");
        }
        result_log["OwnedNodesComeFirst"] = good;
    }

    mp_rootprint("Check all nodes exist in at least 1 cell ");
    good = checkAllNodesExistInACell(mesh);
    good = mp.ParallelAnd(good);
    result_log["AllNodesExistInACell"] = good;
    mp_rootprint(".... %s!\n", Parfait::to_string(good).c_str());

    mp_rootprint("Check for sliver cells ");
    good = checkForSliverCells(mp, mesh, face_neighbors);
    good = mp.ParallelAnd(good);
    result_log["SliverCells"] = good;
    if (good) {
        mp_rootprint(".... %s!\n", Parfait::to_string(good).c_str());
    } else {
        mp_rootprint("\nSliver cell check failed\n");
    }

    Tracer::end("MeshSanityChecker::checkAllSerial");
    return result_log;
}
bool MeshSanityChecker::checkForSliverCells(MessagePasser mp,
                                            const MeshInterface& mesh,
                                            const std::vector<std::vector<int>>& c2c,
                                            double sliver_cell_threshold) {
    auto cell_volume_ratios = inf::SliverCellDetection::calcWorstCellVolumeRatio(mesh, c2c);

    double worst = 1.0;
    int sliver_cell_count = 0;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (not mesh.ownedCell(c)) continue;
        if (mesh.is3DCellType(mesh.cellType(c))) {
            worst = std::min(cell_volume_ratios[c], worst);
            if (cell_volume_ratios[c] < sliver_cell_threshold) {
                sliver_cell_count++;
                inf::Cell cell(mesh, c);
                auto p = cell.averageCenter();
                printf("\nError!! Sliver cell detected at %e %e %e\n", p[0], p[1], p[2]);
                printf("The sliver cell is %1.2e times smaller than it's largest neighbor\n",
                       cell_volume_ratios[c]);
            }
        }
    }
    sliver_cell_count = mp.ParallelSum(sliver_cell_count);
    if (sliver_cell_count > 0) {
        mp_rootprint("!!!!!!!!!!!!!!!ERROR!!!!!!!!!!!!!!!!!\n");
        mp_rootprint("  %d sliver cells were detected\n", sliver_cell_count);
        mp_rootprint("Sliver cells can cause solver errors\n");
        mp_rootprint("This grid is unusable.\n");
        mp_rootprint("!!!!!!!!!!!!!!!ERROR!!!!!!!!!!!!!!!!!\n");
    }
    worst = mp.ParallelMin(worst);
    mp_rootprint("The worst sliver cell ratio is %e\n", worst);
    bool good = sliver_cell_count == 0;
    return good;
}
