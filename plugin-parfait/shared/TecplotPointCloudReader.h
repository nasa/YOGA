#pragma once
#include "Reader.h"
#include <parfait/DataFrame.h>
#include <parfait/DataFrameTools.h>
#include <parfait/LinePlotters.h>

class TecplotPointCloudReader : public Reader {
  public:
    TecplotPointCloudReader(std::string filename) {
        auto extension = Parfait::StringTools::getExtension(filename);
        if (extension == "dat") {
            auto df = Parfait::readTecplot(filename);
            points = Parfait::DataFrameTools::extractPoints(df);
        } else if (extension == "3D") {
            auto df = Parfait::LinePlotters::CSVReader::read(filename, " ");
            points = Parfait::DataFrameTools::extractPoints(df);
        } else PARFAIT_THROW("TecplotPointCloudReader doesn't understand extension \"." + extension + "\"");
    }
    inline long nodeCount() const override { return points.size(); }
    inline long cellCount(inf::MeshInterface::CellType type) const override { return 0; };
    inline std::vector<Parfait::Point<double>> readCoords(long start, long end) const override {
        std::vector<Parfait::Point<double>> out(end - start);
        for (int i = 0; i < int(out.size()); i++) {
            out[i] = points[start + i];
        }
        return out;
    }
    inline std::vector<Parfait::Point<double>> readCoords() const { return points; }
    inline std::vector<int> readCellTags(inf::MeshInterface::CellType type, long start, long end) const override {
        return {};
    }

    inline std::vector<long> readCells(inf::MeshInterface::CellType type, long start, long end) const override {
        return {};
    }

    inline std::vector<inf::MeshInterface::CellType> cellTypes() const override { return {}; }

  public:
    std::vector<Parfait::Point<double>> points;
};