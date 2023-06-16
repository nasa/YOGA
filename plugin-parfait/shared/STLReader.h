#pragma once
#include <parfait/STL.h>
#include <parfait/StitchMesh.h>
#include <t-infinity/STLConverters.h>
#include "Reader.h"

class STLReader : public Reader {
  public:
    inline STLReader(std::string filename);

    inline long nodeCount() const override { return mesh->nodeCount(); }

    inline long cellCount(inf::MeshInterface::CellType type) const override {
        if (type == inf::MeshInterface::TRI_3) return mesh->cellCount(type);
        return 0;
    };

    inline std::vector<Parfait::Point<double>> readCoords() const { return mesh->mesh.points; }

    inline std::vector<Parfait::Point<double>> readCoords(long start, long end) const override {
        std::vector<Parfait::Point<double>> out(end - start);
        for (int i = 0; i < int(out.size()); i++) {
            out[i] = mesh->mesh.points[start + i];
        }
        return out;
    }

    inline std::vector<int> readCellTags(inf::MeshInterface::CellType type, long start, long end) const override {
        std::vector<int> out(end - start, 0);
        return out;
    }

    inline std::vector<long> readCells(inf::MeshInterface::CellType type, long start, long end) const override {
        if (type != inf::MeshInterface::TRI_3) return {};
        long tri_count = end - start;
        std::vector<long> out(3 * tri_count);
        for (long t = 0; t < tri_count; t++) {
            for (int i = 0; i < 3; i++)
                out[3 * t + i] = mesh->mesh.cells[inf::MeshInterface::TRI_3][3 * (start + t) + i];
        }
        return out;
    }

    inline std::vector<inf::MeshInterface::CellType> cellTypes() const override { return {inf::MeshInterface::TRI_3}; }

  private:
    std::shared_ptr<inf::TinfMesh> mesh = nullptr;
};

STLReader::STLReader(std::string filename) {
    auto facets = Parfait::STL::load(filename);
    mesh = inf::STLConverters::stitchSTLToInfinity(facets);
}
