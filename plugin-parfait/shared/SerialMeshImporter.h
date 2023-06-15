#pragma once
#include <t-infinity/MeshInterface.h>
#include <t-infinity/TinfMesh.h>
#include <memory>
#include <numeric>
#include "Reader.h"

class SerialMeshImporter {
  public:
    static std::shared_ptr<inf::MeshInterface> import(const Reader& reader) {
        inf::TinfMeshData mesh_data;
        int nnodes = reader.nodeCount();
        mesh_data.points = reader.readCoords(0, nnodes);
        mesh_data.global_node_id.resize(nnodes);
        std::iota(mesh_data.global_node_id.begin(), mesh_data.global_node_id.end(), 0);
        mesh_data.node_owner.resize(nnodes, 0);
        long cells_so_far = 0;
        for (auto cell_type : reader.cellTypes()) {
            int n = reader.cellCount(cell_type);
            auto cells = reader.readCells(cell_type, 0, n);
            mesh_data.cells[cell_type].resize(cells.size());
            std::copy(cells.begin(), cells.end(), mesh_data.cells[cell_type].begin());
            mesh_data.cell_tags[cell_type] = reader.readCellTags(cell_type, 0, n);
            auto& gids = mesh_data.global_cell_id[cell_type];
            gids.resize(n);
            std::iota(gids.begin(), gids.end(), cells_so_far);
            cells_so_far += n;
            mesh_data.cell_owner[cell_type].resize(n, 0);
        }
        return std::make_shared<inf::TinfMesh>(mesh_data, 0);
    }
};
