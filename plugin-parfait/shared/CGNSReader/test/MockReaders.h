#pragma once
#include <MeshInterface.h>
#include <MessagePasser/MessagePasser.h>
#include <stdexcept>
#include <parfait/Point.h>
#include <vector>

class Reader {
  public:
    long nodeCount() const { return 4; }
    long cellCount() const { return 1; }
    long cellCount(inf::MeshInterface::CellType cell_type) const {
        if (cell_type == inf::MeshInterface::CellType::TETRA_4)
            return 1;
        else
            return 0;
    }
    int cellTypeLength(inf::MeshInterface::CellType cell_type) const {
        if (cell_type != inf::MeshInterface::CellType::TETRA_4)
            throw std::logic_error("mock mesh reader only supports tets.");
        return 4;
    }
    std::vector<inf::MeshInterface::CellType> cellTypes() const { return {inf::MeshInterface::CellType::TETRA_4}; }
    std::vector<long> readCells(inf::MeshInterface::CellType cell_type, long start, long end) const {
        if (start == end) return {};
        if (start != 0 and end != 1) throw std::logic_error("MockMeshReader readCell out of bounds");
        return {0, 1, 2, 3};
    }
    std::vector<int> readCellTags(inf::MeshInterface::CellType cell_type, long start, long end) const {
        if (start == end) return {};
        if (start != 0 and end != 1) throw std::logic_error("MockMeshReader readCell out of bounds");
        return {87};
    }
    std::vector<Parfait::Point<double>> readCoords(long start, long end) const {
        std::vector<Parfait::Point<double>> all_coords = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        std::vector<Parfait::Point<double>> coords_out;
        for (int i = start; i < end; i++) {
            coords_out.push_back(all_coords[i]);
        }
        return coords_out;
    }
};

class TwoTetReader {
  public:
    long nodeCount() const { return 8; }
    long cellCount() const { return 2; }
    long cellCount(inf::MeshInterface::CellType cell_type) const {
        if (cell_type == inf::MeshInterface::CellType::TETRA_4)
            return 2;
        else
            return 0;
    }
    int cellTypeLength(inf::MeshInterface::CellType cell_type) const {
        if (cell_type != inf::MeshInterface::CellType::TETRA_4)
            throw std::logic_error("mock mesh reader only supports tets.");
        return 4;
    }
    std::vector<inf::MeshInterface::CellType> cellTypes() const { return {inf::MeshInterface::CellType::TETRA_4}; }
    std::vector<long> readCells(inf::MeshInterface::CellType cell_type, long start, long end) const {
        std::vector<long> all_cells = {0, 1, 2, 3, 4, 5, 6, 7};
        std::vector<long> out;
        for (int i = start; i < end; i++) {
            for (int j = 0; j < 4; j++) {
                int index = i * 4 + j;
                out.push_back(all_cells[index]);
            }
        }
        return out;
    }
    std::vector<int> readCellTags(inf::MeshInterface::CellType cell_type, long start, long end) const {
        std::vector<int> tags = {0, 1};
        std::vector<int> out;
        for (int i = start; i < end; i++) {
            out.push_back(tags[i]);
        }
        return out;
    }
    std::vector<Parfait::Point<double>> readCoords(long start, long end) const {
        std::vector<Parfait::Point<double>> all_coords = {
            {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        std::vector<Parfait::Point<double>> coords_out;
        for (int i = start; i < end; i++) {
            coords_out.push_back(all_coords[i]);
        }
        return coords_out;
    }
};