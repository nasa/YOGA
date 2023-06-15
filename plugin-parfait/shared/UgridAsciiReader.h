#pragma once
#include <Tracer.h>
#include <parfait/CellWindingConverters.h>
#include <parfait/Point.h>
#include <parfait/StringTools.h>
#include <parfait/UgridReader.h>
#include <t-infinity/MeshInterface.h>
#include <map>
#include <string>
#include <vector>
#include "Reader.h"

class UgridAsciiReader : public Reader {
  public:
    enum { NILL_TAG = -11 };

    UgridAsciiReader(const std::string& filename);

    long nodeCount() const override;

    long cellCount(inf::MeshInterface::CellType type) const override;

    std::vector<Parfait::Point<double>> readCoords() const;

    std::vector<Parfait::Point<double>> readCoords(long start, long end) const override;

    std::vector<int> readCellTags(inf::MeshInterface::CellType type, long start, long end) const override;

    std::vector<long> readCells(inf::MeshInterface::CellType type, long start, long end) const override;

    std::vector<inf::MeshInterface::CellType> cellTypes() const override;

  private:
    std::string filename;
    std::vector<Parfait::Point<double>> points;

    std::map<inf::MeshInterface::CellType, long> element_count;
    std::map<inf::MeshInterface::CellType, std::vector<long>> cells;
    std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
};
