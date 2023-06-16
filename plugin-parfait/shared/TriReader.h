#pragma once
#include <parfait/TriReader.h>

class TriReader : public Reader {
  public:
    inline TriReader(std::string filename);

    inline long nodeCount() const override { return mesh.points.size(); }

    inline long cellCount(inf::MeshInterface::CellType type) const override {
        if (type == inf::MeshInterface::TRI_3) return mesh.triangles.size();
        return 0;
    };

    inline std::vector<Parfait::Point<double>> readCoords() const { return mesh.points; }

    inline std::vector<Parfait::Point<double>> readCoords(long start, long end) const override {
        std::vector<Parfait::Point<double>> out(end - start);
        for (int i = 0; i < int(out.size()); i++) {
            out[i] = mesh.points[start + i];
        }
        return out;
    }

    inline std::vector<int> readCellTags(inf::MeshInterface::CellType type, long start, long end) const override {
        if (type != inf::MeshInterface::TRI_3) return {};
        long tri_count = end - start;
        std::vector<int> out(tri_count);
        for (long t = 0; t < tri_count; t++) {
            out[t] = mesh.tags[start + t];
        }
        return out;
    }

    inline std::vector<long> readCells(inf::MeshInterface::CellType type, long start, long end) const override {
        if (type != inf::MeshInterface::TRI_3) return {};
        long tri_count = end - start;
        std::vector<long> out(3 * tri_count);
        for (long t = 0; t < tri_count; t++) {
            for (int i = 0; i < 3; i++) out[3 * t + i] = mesh.triangles[start + t][i];
        }
        return out;
    }

    inline std::vector<inf::MeshInterface::CellType> cellTypes() const override { return {inf::MeshInterface::TRI_3}; }

  private:
    Parfait::TriMesh mesh;
};

TriReader::TriReader(std::string filename) { mesh = Parfait::TriReader::read(filename); }
