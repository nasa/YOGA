#pragma once
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <t-infinity/TinfMesh.h>
#include "Cell.h"
#include "parfait/PointWriter.h"
#include "Geometry.h"

namespace inf {
namespace experimental {
    struct CellEntry {
        inf::MeshInterface::CellType type;
        int index;
        inline bool operator<(const CellEntry& rhs) const {
            auto left = std::make_tuple(type, index);
            auto right = std::make_tuple(rhs.type, rhs.index);
            return left < right;
        }

        inline bool isNull() const { return index == -1; }
        static CellEntry null() { return CellEntry{inf::MeshInterface::NODE, -1}; }
    };
    class CellView {
      public:
        inline CellView(int* p, size_t l) : ptr(p), m_size(l) {}
        inline int operator[](int i) const { return ptr[i]; }
        int size() const { return m_size; }

      private:
        int* ptr;
        size_t m_size;
    };
    struct CellTypeAndNodeIds {
        inf::MeshInterface::CellType type;
        std::vector<int> node_ids;
    };
    struct Cavity {
        typedef std::vector<int> Face;
        std::vector<Face> exposed_faces;
        std::set<CellEntry> cells;
    };
    class MikeCavity {
      public:
        MikeCavity(int d) : dimension(d) {}
        void addCell(const inf::TinfMesh& mesh, CellEntry cell) {
            inf::Cell c(mesh, cell.type, cell.index, dimension);
            for (int f_id = 0; f_id < c.boundingEntityCount(); f_id++) {
                auto nodes = c.nodesInBoundingEntity(f_id);
                addFace(nodes);
            }
            cells.insert(cell);
        }
        void visualizePoints(const inf::TinfMesh& mesh, std::string filename) {
            std::set<int> nodes_on_cavity_surface;
            for (auto& f : exposed_faces) {
                for (auto n : f) {
                    nodes_on_cavity_surface.insert(n);
                }
            }
            std::vector<Parfait::Point<double>> points;
            for (auto n : nodes_on_cavity_surface) {
                points.push_back(mesh.mesh.points[n]);
            }
            Parfait::PointWriter::write(filename, points);
        }
        void addFace(std::vector<int> f) {
            std::set<int> potential_face(f.begin(), f.end());
            bool found = false;
            for (int i = 0; i < int(exposed_faces.size()); i++) {
                auto& e = exposed_faces[i];
                std::set<int> existing_face(e.begin(), e.end());
                if (potential_face == existing_face) {
                    exposed_faces.erase(exposed_faces.begin() + i);
                    found = true;
                    break;
                }
            }
            if (not found) {
                if (dimension == 3) std::reverse(f.begin(), f.end());
                exposed_faces.push_back(f);
            }
        }
        void addFaces(const std::vector<std::vector<int>>& faces) {
            for (auto& f : faces) {
                addFace(f);
            }
        }
        void cleanup() {
            std::set<int> indices_to_erase;
            for (int index = 0; index < int(exposed_faces.size()); index++) {
                auto& bar = exposed_faces[index];
                if (bar.size() == 2) {
                    int count = 0;
                    for (auto& node : exposed_faces) {
                        if (node.size() == 1) {
                            if (node[0] == bar[0] or node[0] == bar[1]) count++;
                        }
                    }
                    if (count == 2) {
                        indices_to_erase.insert(index);
                    }
                }
            }
            std::vector<std::vector<int>> remaining_faces;
            for (int i = 0; i < int(exposed_faces.size()); i++) {
                if (indices_to_erase.count(i) == 0) {
                    remaining_faces.push_back(exposed_faces[i]);
                }
            }
            exposed_faces = remaining_faces;
        }

        int dimension;
        std::vector<std::vector<int>> exposed_faces;
        std::set<CellEntry> cells;
    };
    // Flagged experimental because:
    // ToDo:
    //   - Right now you have to know the cell any cell you delete isn't resident on any other rank
    //        and we give you no way to check.
    //   - Be able to delete more nodes (or cells) on a partition than you add.
    //      -> This requires sending the gids of any unused gids to each rank
    //      -> So they can decrement any gid greater than that gid.
    //      -> This can be a huge cost if you're deleting a lot of cells.
    //      -> Mike does this using chunks.

    class MeshBuilder {
      public:
        enum Status { SUCCESS = 0, FAILURE = 1 };
        MeshBuilder(MessagePasser mp);
        MeshBuilder(MessagePasser mp, std::shared_ptr<inf::MeshInterface>&& mesh_in);
        MeshBuilder(MessagePasser mp, const inf::MeshInterface& mesh_in);

        MeshBuilder(const MeshBuilder& rhs);

        CellView cell(MeshInterface::CellType type, int type_index) const;

        void resetCountsForOffsets();

        bool isNodeModifiable(int node_id) const;
        bool isCellModifiable(CellEntry cell) const;

        void deleteNode(int node_id);
        void deleteCell(MeshInterface::CellType type, int type_index);
        bool isCellNull(CellEntry cell_entry) const;

        void rebuildNodeToCells();
        void rebuildLocal();

        void insertPoint(Parfait::Point<double> p);

        void throwIfDeletingMoreThanAdding() const;
        void sync();
        void makeNewlyCreatedNodesHaveUniqueGids();
        void makeNewlyCreatedCellsHaveUniqueGids();
        long claimNextCellGid();
        long claimNextNodeGid();

        int addNode(Parfait::Point<double> p);
        CellEntry addCell(MeshInterface::CellType type,
                          const std::vector<int>& cell_nodes,
                          int tag = 0);

        std::vector<CellTypeAndNodeIds> generateCellsToAdd(const MikeCavity& cavity,
                                                           int steiner_node_id);
        void replaceCavity(const MikeCavity& cavity, int steiner_node_id);
        bool replaceCavityTryQuads(const MikeCavity& cavity,
                                   int steiner_node_id,
                                   double cost_threshold = 0.3);

        double smooth(int node_id,
                      std::function<double(Parfait::Point<double>, Parfait::Point<double>)>
                          edge_length_function,
                      inf::geometry::DatabaseInterface* db = nullptr,
                      int max_steps = 10);

        void steinerCavity(const std::vector<std::vector<int>>& faces_pointing_into_cavity);
        void relabelNodeInCells(const CellEntry& cell_entry, int old_node_id, int new_node_id);

        void removeLazyDeletedNodes();
        void removeLazyDeletedCells();
        inline bool isBoundaryType(inf::MeshInterface::CellType type) {
            using T = inf::MeshInterface::CellType;
            static std::set<T> boundary_types_2d = {T::BAR_2, T::NODE};
            static std::set<T> boundary_types_3d = {T::TRI_3, T::QUAD_4};
            if (boundary_types_2d.count(type)) return true;
            if (dimension == 3 and boundary_types_3d.count(type)) return true;
            return false;
        }
        void growCavityAroundPoint(
            int node_id,
            std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
            MikeCavity& cavity,
            bool include_boundaries);
        void growCavityAroundPoint_interior(
            int node_id,
            std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
            MikeCavity& cavity) {
            return growCavityAroundPoint(node_id, calc_edge_length, cavity, false);
        }
        void growCavityAroundPoint_boundary(
            int node_id,
            std::function<double(Parfait::Point<double>, Parfait::Point<double>)> calc_edge_length,
            MikeCavity& cavity) {
            return growCavityAroundPoint(node_id, calc_edge_length, cavity, true);
        }
        std::vector<CellEntry> findNeighborsOfFace(const std::vector<int>& face_nodes);

        static int findLastNotDeletedEntry(const std::set<int>& other_deleted, int last);
        bool hasCellBeenModified(CellEntry c) { return cell_ids_that_have_been_modified.count(c); }

      public:
        MessagePasser mp;
        std::shared_ptr<inf::TinfMesh> mesh;
        int dimension = 3;
        std::vector<bool> is_node_modifiable;
        std::map<inf::MeshInterface::CellType, std::vector<bool>> is_cell_modifiable;

        long original_max_global_node_id;
        long original_max_global_cell_id;
        long next_cell_gid;
        long next_node_gid;

        std::map<inf::MeshInterface::CellType, std::set<int>> unused_cell_type_indices;
        std::vector<std::set<CellEntry>> node_to_cells;
        std::set<int> unused_local_node_ids;
        std::set<long> new_node_gids;
        std::set<long> new_cell_gids;

        std::set<long> unused_cell_gids;
        std::set<long> unused_node_gids;
        std::map<int, std::vector<inf::geometry::GeomAssociation>> node_geom_association;
        geometry::DatabaseInterface* db = nullptr;
        std::set<CellEntry> cell_ids_that_have_been_modified;

        void initializeConnectivity();
        long findLargestGlobalCellId() const;
        long findLargestGlobalNodeId() const;

        void printNodeToCells();
        void printNodes();
        void printCells();
        void compactNodeGids();
        void compactCellGids();
        CellEntry findHostCellId(const Parfait::Point<double>& p, CellEntry start_search_from_cell);
        CellEntry findHostCellIdReallySlow(const Parfait::Point<double>& p);
        bool isPointInCell(const Parfait::Point<double>& p, CellEntry cell);

        void setUpModifiabilityForNodes();
        void rebuildCellResidency();
        Cavity findCavity(const std::set<int>& nodes);
        Status edgeCollapse(int a, int b);
        Status surfaceEdgeSplit(int a, int b);
        std::pair<CellEntry, CellEntry> edgeSurfaceNeighbors(int a, int b);
        bool splitEdgeInSurfaceCell(int a, int b, int pn, CellEntry cell);
        CellEntry findNextWalkingCell(const Parfait::Point<double>& p, CellEntry search_cell);
        bool isNodeOnBoundary(int node_id);
        bool isNodeOnGeometryEdge(int node_id) const;
        bool doesCellContainNodes(const std::vector<int>& cell_nodes,
                                  const std::vector<int>& nodes);
        void markNodesNearPartitionBoundaries();
        void markNodesThatAreCriticalOnGeometry();
        void smoothNodeOnGeomEdge(
            int node_id,
            std::function<double(Parfait::Point<double>, Parfait::Point<double>)>
                edge_length_function,
            geometry::DatabaseInterface* db);
        inf::geometry::GeomAssociation calcEdgeGeomAssociation(int a, int b) const;
        double calcWorstCellCostAtNode(int node_id) const;
        static std::vector<std::vector<int>> windFacesHeadToTail(
            std::vector<std::vector<int>> unwound_faces);
    };
}
}
