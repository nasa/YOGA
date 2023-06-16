#include "TetGenReader.h"
#include <parfait/Throw.h>

TetGenReader::TetGenReader(std::string f) : filename(f) {
    auto extension = Parfait::StringTools::getExtension(filename);
    if (extension == "node" or extension == "ele") {
        filename = Parfait::StringTools::stripExtension(filename);
    }
    printf("Reading TetGen files: %s\n", filename.c_str());
    readNodeFile();
    readEleFile();
}
void TetGenReader::readEleFile() {
    std::string ele_file = filename + ".ele";
    FILE* fp = fopen(ele_file.c_str(), "r");
    PARFAIT_ASSERT(fp != nullptr, "Could not open TetGen ele file: " + ele_file);
    int num_tets;
    int nodes_per_tet;
    int num_attributes;
    fscanf(fp, "%d %d %d\n", &num_tets, &nodes_per_tet, &num_attributes);
    PARFAIT_ASSERT(nodes_per_tet == 4, "TetGen tets have unexpected length: " + std::to_string(nodes_per_tet));
    cells[inf::MeshInterface::TETRA_4].resize(num_tets * 4);
    element_count[inf::MeshInterface::TETRA_4] = num_tets;
    cell_tags[inf::MeshInterface::TETRA_4].resize(num_tets, 0);

    auto& tets = cells.at(inf::MeshInterface::TETRA_4);
    int num_nodes = int(points.size());
    int id, a, b, c, d;
    long beginning = ftell(fp);
    int offset = 1;
    // look for zero index
    for (int t = 0; t < num_tets; t++) {
        fscanf(fp, "%d %d %d %d %d\n", &id, &a, &b, &c, &d);
        if (id == 0) {
            offset = 0;
            break;
        }
    }
    fseek(fp, beginning, SEEK_SET);
    for (int t = 0; t < num_tets; t++) {
        fscanf(fp, "%d %d %d %d %d\n", &id, &a, &b, &c, &d);
        id -= offset;
        a -= offset;
        b -= offset;
        c -= offset;
        d -= offset;
        PARFAIT_ASSERT(id >= 0 and id < num_tets, "Found invalid tet id: " + std::to_string(id));
        PARFAIT_ASSERT(a >= 0 and a < num_nodes, "Found invalid tet node: " + std::to_string(a));
        PARFAIT_ASSERT(b >= 0 and b < num_nodes, "Found invalid tet node: " + std::to_string(b));
        PARFAIT_ASSERT(c >= 0 and c < num_nodes, "Found invalid tet node: " + std::to_string(c));
        PARFAIT_ASSERT(d >= 0 and d < num_nodes, "Found invalid tet node: " + std::to_string(d));
        tets[4 * id + 0] = a;
        tets[4 * id + 1] = b;
        tets[4 * id + 2] = c;
        tets[4 * id + 3] = d;
    }
    fclose(fp);
}
void TetGenReader::readNodeFile() {
    auto node_file = filename + ".node";
    FILE* fp = fopen(node_file.c_str(), "r");
    PARFAIT_ASSERT(fp != nullptr, "Could not open TetGen node file: " + node_file);
    int num_nodes;
    int dim;
    int attributes;
    int boundary_marker;
    fscanf(fp, "%d %d %d %d\n", &num_nodes, &dim, &attributes, &boundary_marker);
    points.resize(num_nodes);
    PARFAIT_ASSERT(boundary_marker == 0 or boundary_marker == 1,
                   "Cannot read node file with num boundary markers: " + std::to_string(boundary_marker));
    double x, y, z;
    int id;
    int marker;
    long beginning = ftell(fp);
    int offset = 1;
    // look for zero index
    for (int t = 0; t < num_nodes; t++) {
        if (boundary_marker == 1)
            fscanf(fp, "%d %lf %lf %lf %d\n", &id, &x, &y, &z, &marker);
        else if (boundary_marker == 0)
            fscanf(fp, "%d %lf %lf %lf\n", &id, &x, &y, &z);
        if (id == 0) {
            offset = 0;
            break;
        }
    }
    if (offset == 0) {
        printf("Tetgen file is zero indexed\n");
    } else {
        printf("Tetgen file is one indexed\n");
    }
    fseek(fp, beginning, SEEK_SET);
    for (int i = 0; i < num_nodes; i++) {
        if (boundary_marker == 1)
            fscanf(fp, "%d %lf %lf %lf %d\n", &id, &x, &y, &z, &marker);
        else if (boundary_marker == 0)
            fscanf(fp, "%d %lf %lf %lf\n", &id, &x, &y, &z);
        id -= offset;
        points[id][0] = x;
        points[id][1] = y;
        points[id][2] = z;
    }
    fclose(fp);
}
long TetGenReader::nodeCount() const { return points.size(); }
long TetGenReader::cellCount(inf::MeshInterface::CellType type) const {
    if (element_count.count(type) == 0)
        return 0;
    else
        return element_count.at(type);
}
std::vector<Parfait::Point<double>> TetGenReader::readCoords() const { return points; }
std::vector<inf::MeshInterface::CellType> TetGenReader::cellTypes() const {
    std::vector<inf::MeshInterface::CellType> types;
    for (auto& pair : element_count) {
        if (pair.second != 0) types.push_back(pair.first);
    }
    return types;
}
std::vector<Parfait::Point<double>> TetGenReader::readCoords(long start, long end) const {
    std::vector<Parfait::Point<double>> out;
    out.reserve(end - start);
    for (long i = start; i < end; i++) {
        out.push_back(points.at(i));
    }
    return out;
}
std::vector<int> TetGenReader::readCellTags(inf::MeshInterface::CellType type, long start, long end) const {
    if (type == inf::MeshInterface::TRI_3) {
        std::vector<int> out;
        out.reserve(end - start);
        for (long i = start; i < end; i++) {
            out.push_back(cell_tags.at(type).at(i));
        }
        return out;

    } else if (type == inf::MeshInterface::QUAD_4) {
        std::vector<int> out;
        out.reserve(end - start);
        for (long i = start; i < end; i++) {
            out.push_back(cell_tags.at(type).at(i));
        }
        return out;
    } else {
        return std::vector<int>(end - start, NILL_TAG);
    }
}
std::vector<long> TetGenReader::readCells(inf::MeshInterface::CellType type, long start, long end) const {
    std::vector<long> out;
    if (end <= start) return out;
    if (cells.count(type) == 0) return out;

    auto length = inf::MeshInterface::cellTypeLength(type);
    auto& elements = cells.at(type);
    out.reserve(length * (end - start));

    for (long e = start; e < end; e++) {
        for (int i = 0; i < length; i++) {
            out.push_back(elements.at(length * e + i));
        }
    }
    return out;
}
