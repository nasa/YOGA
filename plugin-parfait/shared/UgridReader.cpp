#include "UgridReader.h"
bool UgridReader::isBigEndian(std::string name) {
    auto split = Parfait::StringTools::split(name, ".");
    for (const auto& s : split)
        if (s == "b8") return true;
    return false;
}
UgridReader::UgridReader(const std::string& filename)
    : filename(filename),
      swap_bytes(isBigEndian(filename)),
      reader(std::make_unique<Parfait::UgridReader>(filename, swap_bytes)) {
    initializeCounts();
}
long UgridReader::nodeCount() const { return node_count; }
long UgridReader::cellCount(inf::MeshInterface::CellType type) const {
    if (element_count.count(type) == 0)
        return 0;
    else
        return element_count.at(type);
}
std::vector<Parfait::Point<double>> UgridReader::readCoords() const { return readCoords(0, node_count); }
std::vector<Parfait::Point<double>> UgridReader::readCoords(long start, long end) const {
    std::vector<Parfait::Point<double>> points;
    long count = end - start;
    // Tracer::begin("reading " + std::to_string(count) + " nodes");
    auto coords = reader->readNodes(int(start), int(end));
    // Tracer::end("reading " + std::to_string(count) + " nodes");
    points.resize(count);
    for (long index = 0; index < count; index++) {
        points[index] = {coords[3 * index + 0], coords[3 * index + 1], coords[3 * index + 2]};
    }
    return points;
}
std::vector<int> UgridReader::readCellTags(inf::MeshInterface::CellType type, long start, long end) const {
    std::vector<int> tags;
    if (type == inf::MeshInterface::TRI_3)
        tags = reader->readTriangleBoundaryTags(int(start), int(end));
    else if (type == inf::MeshInterface::QUAD_4)
        tags = reader->readQuadBoundaryTags(int(start), int(end));
    else
        tags = std::vector<int>(end - start, NILL_TAG);
    return tags;
}
std::vector<long> UgridReader::readCells(inf::MeshInterface::CellType type, long start, long end) const {
    std::vector<int> int_cells;
    switch (type) {
        case inf::MeshInterface::TRI_3:
            int_cells = reader->readTriangles(int(start), int(end));
            break;
        case inf::MeshInterface::QUAD_4:
            int_cells = reader->readQuads(int(start), int(end));
            break;
        case inf::MeshInterface::TETRA_4:
            int_cells = reader->readTets(int(start), int(end));
            break;
        case inf::MeshInterface::PYRA_5:
            int_cells = reader->readPyramids(int(start), int(end));
            break;
        case inf::MeshInterface::PENTA_6:
            int_cells = reader->readPrisms(int(start), int(end));
            break;
        case inf::MeshInterface::HEXA_8:
            int_cells = reader->readHexs(int(start), int(end));
            break;
        default:
            int_cells = {};
    }

    std::vector<long> cells;
    cells.resize(int_cells.size());
    for (size_t i = 0; i < int_cells.size(); i++) {
        cells[i] = long(int_cells[i]);
    }
    return cells;
}
std::vector<inf::MeshInterface::CellType> UgridReader::cellTypes() const {
    std::vector<inf::MeshInterface::CellType> types;
    for (auto& pair : element_count) {
        if (pair.second != 0) types.push_back(pair.first);
    }
    return types;
}
void UgridReader::initializeCounts() {
    int nnodes, ntri, nquad, ntet, npyr, nprism, nhex;
    reader->readHeader(nnodes, ntri, nquad, ntet, npyr, nprism, nhex);
    node_count = long(nnodes);
    element_count[inf::MeshInterface::TRI_3] = long(ntri);
    element_count[inf::MeshInterface::QUAD_4] = long(nquad);
    element_count[inf::MeshInterface::TETRA_4] = long(ntet);
    element_count[inf::MeshInterface::PYRA_5] = long(npyr);
    element_count[inf::MeshInterface::PENTA_6] = long(nprism);
    element_count[inf::MeshInterface::HEXA_8] = long(nhex);
}
