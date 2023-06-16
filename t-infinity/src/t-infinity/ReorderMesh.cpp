
#include <random>
#include "ReorderMesh.h"

namespace inf {
namespace MeshReorder {

    inline void reorderBasedData(const std::shared_ptr<MeshInterface>& input_mesh,
                                 const std::vector<int>& old_to_new_node_ordering,
                                 TinfMeshData& data) {
        int num_nodes = input_mesh->nodeCount();
        data.points.resize(num_nodes);
        for (int old_node = 0; old_node < num_nodes; old_node++) {
            int new_node = old_to_new_node_ordering[old_node];
            input_mesh->nodeCoordinate(old_node, data.points[new_node].data());
        }

        data.node_owner.resize(num_nodes);
        for (int old_node = 0; old_node < num_nodes; old_node++) {
            int new_node = old_to_new_node_ordering[old_node];
            data.node_owner[new_node] = input_mesh->nodeOwner(old_node);
        }

        data.global_node_id.resize(num_nodes);
        for (int old_node = 0; old_node < num_nodes; old_node++) {
            int new_node = old_to_new_node_ordering[old_node];
            data.global_node_id[new_node] = input_mesh->globalNodeId(old_node);
        }
    }

    void copyUnmodifiedData(const inf::MeshInterface& mesh, TinfMeshData& data) {
        int num_cells = mesh.cellCount();

        data.global_cell_id.clear();
        data.cell_owner.clear();
        for (int c_id = 0; c_id < num_cells; c_id++) {
            auto cell_type = mesh.cellType(c_id);
            data.global_cell_id[cell_type].push_back(mesh.globalCellId(c_id));
            data.cell_owner[cell_type].push_back(mesh.cellOwner(c_id));
        }
    }

    std::shared_ptr<inf::TinfMesh> reorderCells(
        std::shared_ptr<inf::MeshInterface> input_mesh,
        const std::map<inf::MeshInterface::CellType, std::vector<int>>& old_to_new_cell_orderings) {
        TinfMesh old_mesh(input_mesh);
        auto new_mesh = old_mesh.mesh;

        for (auto& type_reorder_pair : old_to_new_cell_orderings) {
            auto type = type_reorder_pair.first;
            auto& old_to_new = type_reorder_pair.second;

            int type_length = inf::MeshInterface::cellTypeLength(type);

            for (int old_id = 0; old_id < int(old_to_new.size()); old_id++) {
                int new_id = old_to_new[old_id];
                new_mesh.cell_owner[type][new_id] = old_mesh.mesh.cell_owner[type][old_id];
                new_mesh.cell_tags[type][new_id] = old_mesh.mesh.cell_tags[type][old_id];
                new_mesh.global_cell_id[type][new_id] = old_mesh.mesh.global_cell_id[type][old_id];

                for (int i = 0; i < type_length; i++) {
                    new_mesh.cells[type][type_length * new_id + i] =
                        old_mesh.mesh.cells[type][type_length * old_id + i];
                }
            }
        }

        return std::make_shared<inf::TinfMesh>(new_mesh, old_mesh.partitionId());
    }

    std::map<int, std::set<int>> buildN2CForJustThisType(const inf::TinfMesh& mesh,
                                                         inf::MeshInterface::CellType type) {
        const auto& cells = mesh.mesh.cells.at(type);
        int length = mesh.cellTypeLength(type);

        std::vector<int> cell_nodes(length);
        std::map<int, std::set<int>> n2c_in_type;

        int num_cells = cells.size() / length;
        for (int index_in_type = 0; index_in_type < num_cells; index_in_type++) {
            for (int i = 0; i < length; i++) {
                int node_id = cells[index_in_type * length + i];
                n2c_in_type[node_id].insert(index_in_type);
            }
        }

        return n2c_in_type;
    }

    std::vector<std::vector<int>> buildC2CForJustThisType(const inf::TinfMesh& mesh,
                                                          inf::MeshInterface::CellType type) {
        auto n2c = buildN2CForJustThisType(mesh, type);
        int length = mesh.cellTypeLength(type);
        const auto& cells = mesh.mesh.cells.at(type);

        int num_cells = cells.size() / length;
        std::vector<std::vector<int>> c2c(num_cells);
        for (int index_in_type = 0; index_in_type < num_cells; index_in_type++) {
            for (int i = 0; i < length; i++) {
                int node_id = cells[index_in_type * length + i];
                const auto& neighbors = n2c.at(node_id);
                for (auto neighbor : neighbors) {
                    if (neighbor != index_in_type) {
                        Parfait::VectorTools::insertUnique(c2c[index_in_type], neighbor);
                    }
                }
            }
        }
        return c2c;
    }

    void throwIfNotPermutation(const std::vector<int>& old_to_new) {
        std::set<int> old_set;
        std::set<int> new_set;

        for (int i = 0; i < int(old_to_new.size()); i++) {
            old_set.insert(i);
            new_set.insert(old_to_new[i]);
        }

        for (int i : old_set) {
            if (new_set.count(i) != 1) {
                PARFAIT_THROW("old to new mapping is missing entry for old id " +
                              std::to_string(i));
            }
        }

        for (int i : new_set) {
            if (old_set.count(i) != 1) {
                PARFAIT_THROW("old to new mapping has extra entry for id " + std::to_string(i));
            }
        }
    }

    int findMin(const std::vector<int>& vec) {
        int min = vec[0];
        for (int i = 1; i < int(vec.size()); i++) {
            min = std::min(min, vec[i]);
        }
        return min;
    }

    std::vector<int> calcNewIdsBySortingMinimumConnected(
        const std::vector<std::vector<int>>& adjacency) {
        std::vector<int> new_order(adjacency.size());
        for (int i = 0; i < int(new_order.size()); i++) {
            new_order[i] = i;
        }
        auto comparator = [&](int a, int b) {
            int amin = findMin(adjacency[a]);
            int bmin = findMin(adjacency[b]);
            return amin < bmin;
        };
        std::sort(new_order.begin(), new_order.end(), comparator);
        std::vector<int> new_ids(adjacency.size());
        for (int i = 0; i < int(adjacency.size()); i++) new_ids[new_order.at(i)] = i;
        return new_ids;
    }

    std::shared_ptr<inf::TinfMesh> reorderNodesBasedOnCells(
        std::shared_ptr<inf::MeshInterface> input_mesh) {
        auto n2c = inf::NodeToCell::build(*input_mesh);
        auto old_to_new_nodes = inf::MeshReorder::calcNewIdsBySortingMinimumConnected(n2c);
        return reorderMeshNodes(input_mesh, old_to_new_nodes);
    }
    std::shared_ptr<inf::TinfMesh> reorderNodesRCM(std::shared_ptr<inf::MeshInterface> input_mesh) {
        if (input_mesh->nodeCount() == 0) return std::make_shared<TinfMesh>(input_mesh);
        auto graph_reorder = [](const std::vector<std::vector<int>>& graph) {
            return inf::ReverseCutthillMckee::calcNewIds(graph);
        };
        return reorderNodes(input_mesh, graph_reorder);
    }
    std::vector<int> getRandomVectorOfOrdinals(int num_ordinals) {
        std::vector<int> ordinals(num_ordinals);
        std::iota(ordinals.begin(), ordinals.end(), 0);

        std::mt19937 gen;
        gen.seed(42);

        std::shuffle(ordinals.begin(), ordinals.end(), gen);
        return ordinals;
    }
    std::shared_ptr<inf::TinfMesh> reorderCellsRandomly(
        std::shared_ptr<inf::MeshInterface> input_mesh) {
        std::shared_ptr<inf::TinfMesh> tinf_mesh = nullptr;
        tinf_mesh = std::dynamic_pointer_cast<inf::TinfMesh>(input_mesh);
        if (tinf_mesh == nullptr) {
            tinf_mesh = std::make_shared<inf::TinfMesh>(input_mesh);
        }

        auto types = Parfait::MapTools::keys(tinf_mesh->mesh.cell_tags);
        std::map<inf::MeshInterface::CellType, std::vector<int>> cell_old_to_new_reordering;
        for (auto type : types) {
            cell_old_to_new_reordering[type] =
                getRandomVectorOfOrdinals(tinf_mesh->cellCount(type));
            throwIfNotPermutation(cell_old_to_new_reordering[type]);
        }

        auto reordered_mesh = inf::MeshReorder::reorderCells(tinf_mesh, cell_old_to_new_reordering);
        return reordered_mesh;
    }
    std::shared_ptr<inf::TinfMesh> reorderNodes(std::shared_ptr<inf::MeshInterface> input_mesh,
                                                Algorithm algorithm,
                                                double p) {
        auto rcm_reorder = [](const std::vector<std::vector<int>>& graph) {
            return ReverseCutthillMckee::calcNewIds(graph);
        };
        auto q_reorder = [=](const std::vector<std::vector<int>>& graph) {
            return ReverseCutthillMckee::calcNewIdsQ(graph, p);
        };
        switch (algorithm) {
            case RCM: {
                return reorderNodes(input_mesh, rcm_reorder);
            }
            case Q: {
                auto rcm_mesh = reorderNodes(input_mesh, rcm_reorder);
                return reorderNodes(rcm_mesh, q_reorder);
            }
            default:
                PARFAIT_THROW("Encountered unexpected reordering algorithm");
        }
    }
    std::shared_ptr<inf::TinfMesh> reorderCells(std::shared_ptr<inf::MeshInterface> input_mesh,
                                                Algorithm algorithm,
                                                double p) {
        switch (algorithm) {
            case RCM: {
                return reorderCellsRCM(input_mesh);
            }
            case RANDOM: {
                return reorderCellsRandomly(input_mesh);
            }
            case Q: {
                auto rcm_reordered = reorderCellsRCM(input_mesh);
                return reorderCellsQ(rcm_reordered, p);
            }
            default:
                PARFAIT_THROW("Encountered unexpected reordering algorithm");
        }
    }
    std::shared_ptr<inf::TinfMesh> reorderCellsQ(std::shared_ptr<inf::MeshInterface> input_mesh,
                                                 double p) {
        std::shared_ptr<inf::TinfMesh> tinf_mesh = nullptr;
        tinf_mesh = std::dynamic_pointer_cast<inf::TinfMesh>(input_mesh);
        if (tinf_mesh == nullptr) {
            tinf_mesh = std::make_shared<inf::TinfMesh>(input_mesh);
        }
        auto types = Parfait::MapTools::keys(tinf_mesh->mesh.cell_tags);
        std::map<inf::MeshInterface::CellType, std::vector<int>> cell_old_to_new_reordering;
        for (auto type : types) {
            auto c2c = buildC2CForJustThisType(*tinf_mesh, type);
            cell_old_to_new_reordering[type] = inf::ReverseCutthillMckee::calcNewIdsQ(c2c, p);
            throwIfNotPermutation(cell_old_to_new_reordering[type]);
        }

        auto reordered_mesh = inf::MeshReorder::reorderCells(tinf_mesh, cell_old_to_new_reordering);
        return reordered_mesh;
    }
    std::shared_ptr<inf::TinfMesh> reorderCellsRCM(std::shared_ptr<inf::MeshInterface> input_mesh) {
        std::shared_ptr<inf::TinfMesh> tinf_mesh = nullptr;
        tinf_mesh = std::dynamic_pointer_cast<inf::TinfMesh>(input_mesh);
        if (tinf_mesh == nullptr) {
            tinf_mesh = std::make_shared<inf::TinfMesh>(input_mesh);
        }
        auto types = Parfait::MapTools::keys(tinf_mesh->mesh.cell_tags);
        std::map<inf::MeshInterface::CellType, std::vector<int>> cell_old_to_new_reordering;
        for (auto type : types) {
            auto c2c = buildC2CForJustThisType(*tinf_mesh, type);
            cell_old_to_new_reordering[type] = inf::ReverseCutthillMckee::calcNewIds(c2c);
            throwIfNotPermutation(cell_old_to_new_reordering[type]);
        }

        auto reordered_mesh = inf::MeshReorder::reorderCells(tinf_mesh, cell_old_to_new_reordering);
        return reordered_mesh;
    }
    void reorderCellNodes(const std::shared_ptr<MeshInterface>& input_mesh,
                          const std::vector<int>& old_to_new_node_ordering,
                          TinfMeshData& data) {
        int num_cells = input_mesh->cellCount();
        std::vector<int> cell;
        for (int c_id = 0; c_id < num_cells; c_id++) {
            auto type = input_mesh->cellType(c_id);
            int length = input_mesh->cellLength(c_id);
            cell.resize(length);
            input_mesh->cell(c_id, cell.data());
            for (auto node : cell) {
                node = old_to_new_node_ordering[node];
                data.cells[type].push_back(node);
            }
            int tag = input_mesh->cellTag(c_id);
            data.cell_tags[type].push_back(tag);
        }
    }
    std::shared_ptr<inf::TinfMesh> reorderMeshNodes(
        std::shared_ptr<inf::MeshInterface> input_mesh,
        const std::vector<int>& old_to_new_node_ordering) {
        TinfMeshData data;
        reorderBasedData(input_mesh, old_to_new_node_ordering, data);
        reorderCellNodes(input_mesh, old_to_new_node_ordering, data);
        copyUnmodifiedData(*input_mesh.get(), data);
        return std::make_shared<inf::TinfMesh>(data, input_mesh->partitionId());
    }

    std::tuple<int, int> findMinAndMax(const std::vector<int>& vec) {
        int min = std::numeric_limits<int>::max();
        int max = std::numeric_limits<int>::lowest();

        for (int a : vec) {
            min = std::min(a, min);
            max = std::max(a, max);
        }
        return {min, max};
    }

    int findMedian(std::vector<int>& vec) {
        size_t size = vec.size();

        if (size == 0)
            return -1;
        else {
            sort(vec.begin(), vec.end());
            if (size % 2 == 0)
                return (vec[size / 2 - 1] + vec[size / 2]) / 2;
            else
                return vec[size / 2];
        }
    }

    int calcIndexArea(const std::vector<int>& vec) {
        int area = 0;
        for (auto a : vec) {
            for (auto b : vec) {
                area += abs(a - b);
            }
        }
        return area;
    }

    std::string reportEstimatedCacheEfficiencyForCells(MessagePasser mp,
                                                       const inf::MeshInterface& mesh) {
        std::vector<int> cell_nodes;

        int diameter_max = 0;
        long diameter_avg = 0;
        int area_max = 0;
        long area_avg = 0;
        long count = 0;

        long across_stencil_diameter_max = 0;
        long across_stencil_diameter_avg = 0;

        int previous_median = -1;
        int across_stencil_count = 0;

        for (int cell = 0; cell < mesh.cellCount(); cell++) {
            if (mesh.ownedCell(cell)) {
                if (mesh.is3DCellType(mesh.cellType(cell))) {
                    mesh.cell(cell, cell_nodes);

                    auto area = calcIndexArea(cell_nodes);
                    area_max = std::max(area_max, area);
                    area_avg += area;

                    int median = findMedian(cell_nodes);
                    if (median != -1 and previous_median != -1) {
                        long sd = abs(previous_median - median);
                        across_stencil_diameter_avg += sd;
                        across_stencil_diameter_max = std::max(sd, across_stencil_diameter_max);
                        across_stencil_count++;
                    }
                    previous_median = median;

                    int min, max;
                    std::tie(min, max) = findMinAndMax(cell_nodes);
                    int diameter = max - min;
                    diameter_max = std::max(diameter_max, diameter);
                    diameter_avg += diameter;

                    count++;
                }
            }
        }

        count = mp.ParallelSum(count);
        diameter_avg = mp.ParallelSum(diameter_avg);
        diameter_max = mp.ParallelMax(diameter_max);
        area_avg = mp.ParallelSum(area_avg);
        area_max = mp.ParallelMax(area_max);
        diameter_avg /= (count);
        area_avg /= (count);

        across_stencil_diameter_avg = mp.ParallelSum(across_stencil_diameter_avg);
        across_stencil_diameter_max = mp.ParallelMax(across_stencil_diameter_max);
        across_stencil_count = mp.ParallelSum(across_stencil_count);
        across_stencil_diameter_avg /= across_stencil_count;

        std::string msg = "Cache Efficiency estimates: Smaller is better.\n";
        msg +=
            "Cache Diameter of a cell measured as: \n    Diameter = max_node_index - "
            "min_node_index\n";
        msg += "Maximum Cache Diameter: " + std::to_string(diameter_max) + "\n";
        msg += "Average Cache Diameter: " + std::to_string(diameter_avg) + "\n";
        msg +=
            "Cache Area of a cell, measured as: \n    Area = Sum over i,j { abs(node_index_i - "
            "node_index_j) }\n";
        msg += "Maximum Cache Area    : " + std::to_string(area_max) + "\n";
        msg += "Average Cache Area    : " + std::to_string(area_avg) + "\n";

        msg +=
            "Cache Distance Between cells, measured as: \n    Dist = abs(median_index_cell_(i) - "
            "median_index_cell_(i+1))\n";
        msg += "Maximum Cache Dist    : " + std::to_string(across_stencil_diameter_max) + "\n";
        msg += "Average Cache Dist    : " + std::to_string(across_stencil_diameter_avg) + "\n";

        mp_rootprint("%s\n", msg.c_str());
        return msg;
    }
    std::vector<int> buildReorderNodeOwnedFirst(std::vector<bool>& do_own) {
        std::vector<int> owned;
        std::vector<int> ghost;

        for (size_t n = 0; n < do_own.size(); n++) {
            if (do_own[n])
                owned.push_back(n);
            else
                ghost.push_back(n);
        }

        owned.insert(owned.end(), ghost.begin(), ghost.end());
        return owned;
    }
    void writeAdjacency(std::string filename, const std::vector<std::vector<int>>& adjacency) {
        FILE* fp = fopen(filename.c_str(), "w");
        for (int i = 0; i < int(adjacency.size()); i++) {
            for (int j = 0; j < int(adjacency[i].size()); j++) {
                fprintf(fp, "%d %d\n", i, adjacency[i][j]);
            }
        }
        fclose(fp);
    }
    std::shared_ptr<inf::TinfMesh> reorderNodes(
        std::shared_ptr<inf::MeshInterface> input_mesh,
        std::function<std::vector<int>(const std::vector<std::vector<int>>&)> graph_reorder) {
        auto n2n = inf::NodeToNode::build(*input_mesh);
        auto old_to_new_nodes = graph_reorder(n2n);
        return reorderMeshNodes(input_mesh, old_to_new_nodes);
    }
}
}
