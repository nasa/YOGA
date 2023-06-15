#pragma once
#include <parfait/Point.h>
#include <t-infinity/MeshInterface.h>

class Reader {
  public:
    virtual long nodeCount() const = 0;
    virtual long cellCount(inf::MeshInterface::CellType etype) const = 0;
    virtual std::vector<Parfait::Point<double>> readCoords(long start, long end) const = 0;

    virtual std::vector<long> readCells(inf::MeshInterface::CellType type, long start, long end) const = 0;
    virtual std::vector<int> readCellTags(inf::MeshInterface::CellType element_type,
                                          long element_start,
                                          long element_end) const = 0;
    virtual std::vector<inf::MeshInterface::CellType> cellTypes() const = 0;

    virtual ~Reader() = default;
};
