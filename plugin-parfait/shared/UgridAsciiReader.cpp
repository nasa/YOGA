
#include "UgridAsciiReader.h"

UgridAsciiReader::UgridAsciiReader(const std::string& filename) : filename(filename) {
    FILE* fp = fopen(filename.c_str(), "r");
    if (fp == NULL) {
        throw std::domain_error("Could not open .ugrid file: " + filename);
    }
    int num_nodes, num_tris, num_quads, num_tets, num_pyramids, num_prisms, num_hexs;
    fscanf(fp,
           "%d %d %d %d %d %d %d\n",
           &num_nodes,
           &num_tris,
           &num_quads,
           &num_tets,
           &num_pyramids,
           &num_prisms,
           &num_hexs);

    points.resize(num_nodes);
    for (int i = 0; i < num_nodes; i++) {
        fscanf(fp, "%lf %lf %lf", &points[i][0], &points[i][1], &points[i][2]);
    }

    if (num_tris != 0) {
        auto type = inf::MeshInterface::TRI_3;
        int count = num_tris;
        int length = 3;
        element_count[type] = count;
        cells[type].resize(length * count);
        auto& triangles = cells[type];
        for (int cell_id = 0; cell_id < count; cell_id++) {
            for (int i = 0; i < length; i++) {
                fscanf(fp, "%ld", &triangles[3 * cell_id + i]);
                triangles[3 * cell_id + i]--;
            }
        }
    }
    if (num_quads != 0) {
        auto type = inf::MeshInterface::QUAD_4;
        element_count[type] = num_quads;
        cells[type].resize(4 * num_quads);
        auto& quads = cells[type];
        for (int quadId = 0; quadId < num_quads; quadId++) {
            for (int i = 0; i < 4; i++) {
                fscanf(fp, "%ld", &quads[4 * quadId + i]);
                quads[4 * quadId + i]--;
            }
        }
    }

    if (num_tris != 0) {
        auto& tags = cell_tags[inf::MeshInterface::TRI_3];
        tags.resize(num_tris);
        for (int triId = 0; triId < num_tris; triId++) {
            fscanf(fp, "%d", &tags[triId]);
        }
    }
    if (num_quads != 0) {
        auto& tags = cell_tags[inf::MeshInterface::QUAD_4];
        tags.resize(num_quads);
        for (int quadId = 0; quadId < num_quads; quadId++) {
            fscanf(fp, "%d", &tags[quadId]);
        }
    }

    if (num_tets != 0) {
        auto type = inf::MeshInterface::TETRA_4;
        element_count[type] = num_tets;
        cells[type].resize(4 * num_tets);
        auto& tets = cells[type];
        for (int elem = 0; elem < num_tets; elem++) {
            for (int i = 0; i < 4; i++) {
                fscanf(fp, "%ld", &tets[4 * elem + i]);
                tets[4 * elem + i]--;
            }
        }
    }
    if (num_pyramids != 0) {
        auto type = inf::MeshInterface::PYRA_5;
        element_count[type] = num_pyramids;
        cells[type].resize(5 * num_pyramids);
        auto& pyramids = cells[type];
        for (int elem = 0; elem < num_pyramids; elem++) {
            for (int i = 0; i < 5; i++) {
                fscanf(fp, "%ld", &pyramids[5 * elem + i]);
                pyramids[5 * elem + i]--;
            }
            Parfait::AflrToCGNS::convertPyramid(&pyramids[5 * elem]);
        }
    }
    if (num_prisms != 0) {
        auto type = inf::MeshInterface::PENTA_6;
        element_count[type] = num_prisms;
        cells[type].resize(6 * num_prisms);
        auto& prisms = cells[type];
        for (int elem = 0; elem < num_prisms; elem++) {
            for (int i = 0; i < 6; i++) {
                fscanf(fp, "%ld", &prisms[6 * elem + i]);
                prisms[6 * elem + i]--;
            }
        }
    }
    if (num_hexs != 0) {
        auto type = inf::MeshInterface::HEXA_8;
        element_count[type] = num_hexs;
        cells[type].resize(8 * num_hexs);
        auto& hexs = cells[type];
        for (int elem = 0; elem < num_hexs; elem++) {
            for (int i = 0; i < 8; i++) {
                fscanf(fp, "%ld", &hexs[8 * elem + i]);
                hexs[8 * elem + i]--;
            }
        }
    }
    fclose(fp);
}

long UgridAsciiReader::nodeCount() const { return points.size(); }
long UgridAsciiReader::cellCount(inf::MeshInterface::CellType type) const {
    if (element_count.count(type) == 0)
        return 0;
    else
        return element_count.at(type);
}
std::vector<Parfait::Point<double>> UgridAsciiReader::readCoords() const { return points; }
std::vector<inf::MeshInterface::CellType> UgridAsciiReader::cellTypes() const {
    std::vector<inf::MeshInterface::CellType> types;
    for (auto& pair : element_count) {
        if (pair.second != 0) types.push_back(pair.first);
    }
    return types;
}
std::vector<Parfait::Point<double>> UgridAsciiReader::readCoords(long start, long end) const {
    std::vector<Parfait::Point<double>> out;
    out.reserve(end - start);
    for (long i = start; i < end; i++) {
        out.push_back(points[i]);
    }
    return out;
}
std::vector<int> UgridAsciiReader::readCellTags(inf::MeshInterface::CellType type, long start, long end) const {
    if (type == inf::MeshInterface::TRI_3) {
        std::vector<int> out;
        out.reserve(end - start);
        for (long i = start; i < end; i++) {
            out.push_back(cell_tags.at(type)[i]);
        }
        return out;

    } else if (type == inf::MeshInterface::QUAD_4) {
        std::vector<int> out;
        out.reserve(end - start);
        for (long i = start; i < end; i++) {
            out.push_back(cell_tags.at(type)[i]);
        }
        return out;
    } else {
        return std::vector<int>(end - start, NILL_TAG);
    }
}
std::vector<long> UgridAsciiReader::readCells(inf::MeshInterface::CellType type, long start, long end) const {
    std::vector<long> out;
    if (end <= start) return out;
    if (cells.count(type) == 0) return out;

    auto length = inf::MeshInterface::cellTypeLength(type);
    auto& elements = cells.at(type);
    out.reserve(length * (end - start));

    for (long e = start; e < end; e++) {
        for (int i = 0; i < length; i++) {
            out.push_back(elements[length * e + i]);
        }
    }
    return out;
}
