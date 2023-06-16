#pragma once
#include <map>
#include <string>
#include <parfait/StringTools.h>
#include <t-infinity/MeshInterface.h>
#include "Reader.h"

class TetGenReader : public Reader {
  private:
    enum { NILL_TAG = -11 };
    std::string filename;
    std::vector<Parfait::Point<double>> points;

    std::map<inf::MeshInterface::CellType, long> element_count;
    std::map<inf::MeshInterface::CellType, std::vector<long>> cells;
    std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;

  public:
    TetGenReader(std::string f);
    void readEleFile();
    void readNodeFile();
    long nodeCount() const;
    long cellCount(inf::MeshInterface::CellType type) const;
    std::vector<Parfait::Point<double>> readCoords() const;
    std::vector<inf::MeshInterface::CellType> cellTypes() const;
    std::vector<Parfait::Point<double>> readCoords(long start, long end) const;
    std::vector<int> readCellTags(inf::MeshInterface::CellType type, long start, long end) const;
    std::vector<long> readCells(inf::MeshInterface::CellType type, long start, long end) const;
};
