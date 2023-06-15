#pragma once

#include <MessagePasser/MessagePasser.h>
#include <NaiveMesh.h>

namespace OneTetMesh {
inline Santa::NaiveMesh oneTetMesh(MessagePasser mp) {
    if (mp.NumberOfProcesses() != 2) {
        std::vector<Parfait::Point<double>> coords = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        std::map<inf::MeshInterface::CellType, std::vector<long>> cells;
        std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
        cells[inf::MeshInterface::TETRA_4] = {0, 1, 2, 3};
        cell_tags[inf::MeshInterface::TETRA_4] = {1};
        std::vector<long> global_node_id = {0, 1, 2, 3};
        Parfait::LinearPartitioner::Range<long> range{0, 4};
        return Santa::NaiveMesh(coords, cells, cell_tags, global_node_id, range);
    } else {
        if (mp.Rank() == 0) {
            std::vector<Parfait::Point<double>> coords = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
            std::map<inf::MeshInterface::CellType, std::vector<long>> cells;
            std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
            cells[inf::MeshInterface::TETRA_4] = {0, 1, 2, 3};
            cell_tags[inf::MeshInterface::TETRA_4] = {1};
            std::vector<long> global_node_id = {0, 1, 2, 3};
            Parfait::LinearPartitioner::Range<long> range{0, 4};
            return Santa::NaiveMesh(coords, cells, cell_tags, global_node_id, range);
        } else if (mp.Rank() == 1) {
            std::vector<Parfait::Point<double>> coords = {{0, 0, 2}, {1, 0, 2}, {0, 1, 2}, {0, 0, 3}};
            std::map<inf::MeshInterface::CellType, std::vector<long>> cells;
            std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
            cells[inf::MeshInterface::TETRA_4] = {4, 5, 6, 7};
            cell_tags[inf::MeshInterface::TETRA_4] = {1};
            std::vector<long> global_node_id = {4, 5, 6, 7};
            Parfait::LinearPartitioner::Range<long> range{4, 8};
            return Santa::NaiveMesh(coords, cells, cell_tags, global_node_id, range);
        }
    }
}
}