#pragma once
#include "VTKWriter.h"
#include "VTKElements.h"
#include "ByteSwap.h"

namespace Parfait {
namespace vtk {

    inline Writer::Writer(std::string filename,
                          long num_points,
                          long num_cells,
                          std::function<void(long, double*)> getPoint,
                          std::function<CellType(int)> getVTKCellType,
                          std::function<void(int, int*)> getCellNodes)
        : filename(std::move(filename)),
          num_points(num_points),
          num_cells(num_cells),
          getPoint(std::move(getPoint)),
          getVTKCellType(std::move(getVTKCellType)),
          getCellNodes(std::move(getCellNodes)),
          fp(nullptr) {}

    inline void Writer::addCellField(const std::string& name, std::function<double(int)> f) {
        cell_scalars[name] = std::move(f);
    }
    inline void Writer::addNodeField(const std::string& name, std::function<double(int)> f) {
        node_scalars[name] = std::move(f);
    }
    inline void Writer::visualize() {
        open();
        writeHeader(fp);
        writePoints();
        writeCells();
        writeCellFields();
        writeNodeFields();
        fclose(fp);
    }
    inline void Writer::writeNodeFields() {
        fprintf(fp, "\nPOINT_DATA %li\n", num_points);
        for (auto& pair : node_scalars) {
            writeNodeField(pair.first, pair.second);
        }
        for (auto& pair : node_tensors) {
            writeNodeField(pair.first, pair.second);
        }
    }
    inline void Writer::writeCellFields() {
        fprintf(fp, "\nCELL_DATA %li\n", num_cells);
        for (auto& pair : cell_scalars) {
            writeCellField(pair.first, pair.second);
        }
        for (auto& pair : cell_tensors) {
            writeCellField(pair.first, pair.second);
        }
    }
    inline void Writer::open() {
        if (Parfait::StringTools::getExtension(filename) != "vtk") filename += ".vtk";
        fp = fopen(filename.c_str(), "w");
        if (fp == nullptr) PARFAIT_THROW("Could not open file for writing: " + filename);
    }
    inline void Writer::writePoints() {
        fprintf(fp, "POINTS %ld double\n", num_points);
        Parfait::Point<double> p;
        for (long node = 0; node < num_points; node++) {
            getPoint(node, p.data());
            bswap_64(&p[0]);
            bswap_64(&p[1]);
            bswap_64(&p[2]);
            fwrite(p.data(), sizeof(double), 3, fp);
        }
    }
    inline size_t Writer::calcCellBufferSize() {
        size_t buffer_size = num_cells;
        for (long cell_id = 0; cell_id < num_cells; cell_id++) {
            int type = getVTKCellType(cell_id);
            buffer_size += getNumPointsFromVTKType(type);
        }

        return buffer_size;
    }
    inline void Writer::rewindCellNodes(CellType type, std::vector<int>& cell_nodes) {
        switch (type) {
            case NODE:
                return;
            case EDGE:
                return;
            case TRI:
                return;
            case QUAD:
                return;
            case QUAD_8:
                return;
            case TRI_P2:
                return;
            case TET_P2:
                return;
            case PENTA_18:
                Parfait::CGNSToVtk::convertPenta_18(cell_nodes.data());
                return;
            case HEX:
                Parfait::CGNSToVtk::convertHex(cell_nodes.data());
                return;
            case HEX_P2:
                Parfait::CGNSToVtk::convertHexa_20(cell_nodes.data());
                return;
            case TET:
                Parfait::CGNSToVtk::convertTet(cell_nodes.data());
                return;
            case PYRAMID:
                Parfait::CGNSToVtk::convertPyramid(cell_nodes.data());
                return;
            case PRISM:
                Parfait::CGNSToVtk::convertPrism(cell_nodes.data());
                return;
            default:
                PARFAIT_THROW("Element type is unknown: " + std::to_string(type));
        }
    }
    inline void Writer::writeCells() {
        size_t cell_buffer_size = calcCellBufferSize();
        fprintf(fp, "\nCELLS %li %zu\n", num_cells, cell_buffer_size);

        int num_cell_points = 18;
        int num_points_big_endian;
        std::vector<int> cell_nodes(num_cell_points);
        for (long cell_id = 0; cell_id < num_cells; cell_id++) {
            CellType cell_type = getVTKCellType(cell_id);
            num_cell_points = getNumPointsFromVTKType(cell_type);
            num_points_big_endian = num_cell_points;
            bswap_32(&num_points_big_endian);
            fwrite(&num_points_big_endian, sizeof(num_cell_points), 1, fp);
            cell_nodes.resize(num_cell_points);
            getCellNodes(cell_id, cell_nodes.data());
            rewindCellNodes(cell_type, cell_nodes);
            cell_nodes.resize(num_cell_points);
            for (int local_node = 0; local_node < num_cell_points; local_node++) {
                bswap_32(&cell_nodes[local_node]);
            }
            fwrite(cell_nodes.data(), sizeof(int), num_cell_points, fp);
        }

        fprintf(fp, "\nCELL_TYPES %li\n", num_cells);
        for (int cell_id = 0; cell_id < num_cells; cell_id++) {
            int cell_type = getVTKCellType(cell_id);
            bswap_32(&cell_type);
            fwrite(&cell_type, sizeof(cell_type), 1, fp);
        }
    }
    inline void Writer::writeCellField(std::string field_name, std::function<double(int)> field) {
        field_name = Parfait::StringTools::findAndReplace(field_name, " ", "-");
        fprintf(fp, "SCALARS %s DOUBLE 1\n", field_name.c_str());
        fprintf(fp, "LOOKUP_TABLE default\n");
        for (long cell_id = 0; cell_id < num_cells; cell_id++) {
            double f = field(cell_id);
            bswap_64(&f);
            fwrite(&f, sizeof(f), 1, fp);
        }
    }
    inline void Writer::writeCellField(std::string field_name, std::function<void(int, double*)> field) {
        field_name = Parfait::StringTools::findAndReplace(field_name, " ", "-");
        fprintf(fp, "TENSORS %s DOUBLE\n", field_name.c_str());
        std::array<double, 6> upper_triangular;
        std::array<std::array<double, 3>, 3> square_matrix;
        for (long cell_id = 0; cell_id < num_cells; cell_id++) {
            field(cell_id, upper_triangular.data());
            square_matrix[0][0] = upper_triangular[0];
            square_matrix[0][1] = upper_triangular[1];
            square_matrix[0][2] = upper_triangular[2];
            square_matrix[1][1] = upper_triangular[3];
            square_matrix[1][2] = upper_triangular[4];
            square_matrix[2][2] = upper_triangular[5];
            square_matrix[1][0] = square_matrix[0][1];
            square_matrix[2][0] = square_matrix[0][2];
            square_matrix[2][1] = square_matrix[1][2];
            for (auto& row : square_matrix) {
                for (double& entry : row) {
                    bswap_64(&entry);
                    fwrite(&entry, sizeof(entry), 1, fp);
                }
            }
        }
    }
    inline void Writer::writeNodeField(std::string field_name, std::function<double(int)> field) {
        field_name = Parfait::StringTools::findAndReplace(field_name, " ", "-");
        fprintf(fp, "SCALARS %s DOUBLE 1\n", field_name.c_str());
        fprintf(fp, "LOOKUP_TABLE DEFAULT\n");
        for (long node_id = 0; node_id < num_points; node_id++) {
            double f = field(node_id);
            bswap_64(&f);
            fwrite(&f, sizeof(f), 1, fp);
        }
    }

    inline void Writer::writeNodeField(std::string field_name, std::function<void(int, double*)> field) {
        field_name = Parfait::StringTools::findAndReplace(field_name, " ", "-");
        fprintf(fp, "TENSORS %s DOUBLE\n", field_name.c_str());
        std::array<double, 6> upper_triangular;
        std::array<std::array<double, 3>, 3> square_matrix;
        for (long node_id = 0; node_id < num_points; node_id++) {
            field(node_id, upper_triangular.data());
            square_matrix[0][0] = upper_triangular[0];
            square_matrix[0][1] = upper_triangular[1];
            square_matrix[0][2] = upper_triangular[2];
            square_matrix[1][1] = upper_triangular[3];
            square_matrix[1][2] = upper_triangular[4];
            square_matrix[2][2] = upper_triangular[5];
            square_matrix[1][0] = square_matrix[0][1];
            square_matrix[2][0] = square_matrix[0][2];
            square_matrix[2][1] = square_matrix[1][2];
            for (auto& row : square_matrix) {
                for (double& entry : row) {
                    bswap_64(&entry);
                    fwrite(&entry, sizeof(entry), 1, fp);
                }
            }
        }
    }

    inline void Writer::close() {
        if (fp) {
            fclose(fp);
            fp = nullptr;
        }
    }
    inline void Writer::addCellTensorField(const std::string& name, std::function<void(int, double*)> f) {
        cell_tensors[name] = f;
    }

    inline void Writer::addNodeTensorField(const std::string& name, std::function<void(int, double*)> f) {
        node_tensors[name] = f;
    }

    inline std::string Writer::getHeader() {
        std::string header = "# vtk DataFile Version 3.0\n";
        header += "Powered by T-infinity\n";
        header += "BINARY\n";
        header += "DATASET UNSTRUCTURED_GRID\n";
        return header;
    }
    inline void Writer::writeHeader(FILE*& f) {
        auto header = getHeader();
        fprintf(f, "%s", header.c_str());
    }
}
}
