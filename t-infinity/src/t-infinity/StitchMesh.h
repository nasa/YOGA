#include <t-infinity/CartMesh.h>
#include <parfait/UniquePoints.h>
#include <parfait/ExtentBuilder.h>
#include <t-infinity/TinfMeshAppender.h>
#include <t-infinity/InternalFaceRemover.h>

namespace inf {
namespace StitchMesh {

    typedef std::pair<inf::MeshInterface::CellType, int> TypeAndIndex;

    inline Parfait::UniquePoints makeUniquePoints(
        const std::vector<std::shared_ptr<MeshInterface>>& meshes) {
        auto domain = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for (auto m : meshes) {
            auto e = Parfait::ExtentBuilder::buildExtentForPointsInMesh(*m);
            Parfait::ExtentBuilder::add(domain, e);
        }
        Parfait::UniquePoints unique_points(domain);
        for (auto& m : meshes) {
            for (int n = 0; n < m->nodeCount(); n++) {
                Parfait::Point<double> p = m->node(n);
                unique_points.getNodeId(p);
            }
        }
        return unique_points;
    }

    inline std::vector<Parfait::Point<double>> getCellPoints(const inf::MeshInterface& mesh,
                                                             int c) {
        std::vector<int> cell_nodes;
        mesh.cell(c, cell_nodes);
        std::vector<Parfait::Point<double>> points;
        for (int n : cell_nodes) {
            points.push_back(mesh.node(n));
        }
        return points;
    }

    inline std::vector<long> getCellStitchedNode(
        Parfait::UniquePoints& unique_points,
        const std::vector<Parfait::Point<double>>& cell_points) {
        std::vector<long> cell_nodes(cell_points.size());
        for (int i = 0; i < int(cell_points.size()); i++) {
            cell_nodes[i] = unique_points.getNodeId(cell_points[i]);
        }
        return cell_nodes;
    }

    template <typename C1, typename C2>
    bool areCellsSame(const C1& cell_1, const C2& cell_2) {
        if (cell_1.size() != cell_2.size()) return false;
        std::set<long> nodes1;
        std::set<long> nodes2;
        for (auto n : cell_1) {
            nodes1.insert(long(n));
        }
        for (auto n : cell_2) {
            nodes2.insert(long(n));
        }
        return nodes1 == nodes2;
    }

    inline bool alreadyHaveCell(inf::MeshInterface::CellType cell_type,
                                const std::vector<long>& cell_nodes,
                                TinfMesh& mesh,
                                const std::vector<std::set<TypeAndIndex>>& n2c) {
        auto first_node = cell_nodes[0];
        auto& neighbors = n2c.at(first_node);
        for (auto n : neighbors) {
            auto type = n.first;
            if (cell_type != type) continue;
            auto index = n.second;
            auto length = inf::MeshInterface::cellTypeLength(type);
            std::vector<int> neighbor_nodes(length);
            for (int i = 0; i < length; i++) {
                neighbor_nodes[i] = mesh.mesh.cells[type][length * index + i];
            }
            if (areCellsSame(neighbor_nodes, cell_nodes)) {
                return true;
            }
        }
        return false;
    }

    inline std::pair<MeshInterface::CellType, int> addCell(TinfMesh& mesh,
                                                           MeshInterface::CellType type,
                                                           std::vector<long>& nodes,
                                                           int tag,
                                                           long next_gcid) {
        int index = mesh.mesh.cell_tags[type].size();
        for (long n : nodes) {
            mesh.mesh.cells[type].push_back(int(n));
        }
        mesh.mesh.cell_tags[type].push_back(tag);
        mesh.mesh.cell_owner[type].push_back(0);
        mesh.mesh.global_cell_id[type].push_back(next_gcid);
        return {type, index};
    }

    inline std::shared_ptr<inf::TinfMesh> stitchMeshes(
        const std::vector<std::shared_ptr<MeshInterface>>& meshes) {
        Parfait::UniquePoints unique_points = makeUniquePoints(meshes);

        std::vector<std::set<TypeAndIndex>> n2c(unique_points.points.size());
        auto appender = std::make_shared<inf::TinfMesh>();
        appender->mesh.points = unique_points.points;

        long next_gcid = 0;
        long next_gnid = 0;
        int fake_partition_id = 0;
        for (size_t i = 0; i < appender->mesh.points.size(); i++)
            appender->mesh.global_node_id.push_back(next_gnid++);
        appender->mesh.node_owner.resize(appender->mesh.points.size(), fake_partition_id);

        for (auto m : meshes) {
            for (int c = 0; c < m->cellCount(); c++) {
                auto points = getCellPoints(*m, c);
                auto nodes = getCellStitchedNode(unique_points, points);
                auto type = m->cellType(c);
                if (alreadyHaveCell(type, nodes, *appender, n2c)) {
                    continue;
                }
                std::vector<int> node_owner(nodes.size(), 0);
                auto type_and_index = addCell(*appender, type, nodes, m->cellTag(c), next_gcid++);
                for (auto n : nodes) {
                    n2c[n].insert(type_and_index);
                }
            }
        }

        appender->rebuild();
        return inf::InternalFaceRemover::remove(MPI_COMM_SELF, appender);
    }
}
}
