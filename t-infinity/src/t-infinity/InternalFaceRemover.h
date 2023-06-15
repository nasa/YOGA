#pragma once
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/CellIdFilter.h>
#include <numeric>

namespace inf {
class InternalFaceRemover {
  public:
    static inline std::shared_ptr<inf::TinfMesh> remove(MPI_Comm comm,
                                                        std::shared_ptr<inf::MeshInterface> mesh) {
        auto ids = identifyInternalFaces(*mesh);
        return std::make_shared<inf::TinfMesh>(removeCells(comm, mesh, ids));
    }
    static inline std::shared_ptr<inf::MeshInterface> removeCells(
        MPI_Comm comm, std::shared_ptr<inf::MeshInterface> mesh, const std::vector<int> ids) {
        auto keep_cells = invertCellSelection(*mesh, ids);
        inf::CellIdFilter filter(comm, mesh, keep_cells);
        return filter.getMesh();
    }
    static inline std::vector<int> identifyInternalFaces(const inf::MeshInterface& mesh) {
        auto n2c = inf::NodeToCell::build(mesh);
        std::vector<int> internal_faces;
        std::set<int> nbrs;
        std::vector<int> cell, face;
        for (int i = 0; i < mesh.cellCount(); i++) {
            if (mesh.is2DCell(i)) {
                nbrs.clear();
                face.resize(mesh.cellLength(i));
                mesh.cell(i, face.data());
                for (int node_id : face) {
                    auto& candidates = n2c[node_id];
                    nbrs.insert(candidates.begin(), candidates.end());
                }
                nbrs.erase(i);
                int nbr_count = 0;
                for (int nbr : nbrs) {
                    cell.resize(mesh.cellLength(nbr));
                    mesh.cell(nbr, cell.data());
                    if (inf::FaceNeighbors::cellContainsFace(cell, face)) {
                        nbr_count++;
                    }
                }
                if (nbr_count == 2) {
                    internal_faces.push_back(i);
                }
            }
        }
        return internal_faces;
    }

  private:
    static std::vector<int> invertCellSelection(const inf::MeshInterface& mesh,
                                                const std::vector<int>& ids) {
        std::vector<int> keep_cells(mesh.cellCount());
        std::iota(keep_cells.begin(), keep_cells.end(), 0);
        std::set<int> ids_to_remove(ids.begin(), ids.end());
        auto predicate = [&](int id) { return ids_to_remove.count(id); };
        auto it = std::remove_if(keep_cells.begin(), keep_cells.end(), predicate);
        keep_cells.resize(it - keep_cells.begin());
        return keep_cells;
    }
};

}