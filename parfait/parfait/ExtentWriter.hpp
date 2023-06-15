#pragma once
#include <parfait/VTKWriter.h>
#include <stdio.h>
#include <numeric>
#include <string>
#include <vector>

namespace Parfait {

namespace ExtentWriter {
    inline void write(std::string filename, const std::vector<Parfait::Extent<double>>& extents) {
        auto get_point = [&](int node_id, double* p_out) {
            int extent_index = node_id / 8;
            int point_index = node_id % 8;
            auto points = extents[extent_index].corners();
            auto p = points[point_index];
            p_out[0] = p[0];
            p_out[1] = p[1];
            p_out[2] = p[2];
        };

        auto get_cell_type = [](int cell_id) {
            vtk::Writer::CellType hex = Parfait::vtk::Writer::HEX;
            return hex;
        };

        auto get_cell_nodes = [](int cell_id, int* node_ids) {
            for (int i = 0; i < 8; i++) {
                int node_id = 8 * cell_id + i;
                node_ids[i] = node_id;
            }
        };

        Parfait::vtk::Writer writer(
            std::move(filename), 8 * extents.size(), extents.size(), get_point, get_cell_type, get_cell_nodes);
        writer.visualize();
    }

    inline void write(std::string filename,
                      const std::vector<Parfait::Extent<double>>& extents,
                      const std::vector<double>& field) {
        auto get_point = [&](int node_id, double* p_out) {
            int extent_index = node_id / 8;
            int point_index = node_id % 8;
            auto points = extents[extent_index].corners();
            auto p = points[point_index];
            p_out[0] = p[0];
            p_out[1] = p[1];
            p_out[2] = p[2];
        };

        auto get_cell_type = [](int cell_id) {
            vtk::Writer::CellType hex = Parfait::vtk::Writer::HEX;
            return hex;
        };

        auto get_cell_nodes = [](int cell_id, int* node_ids) {
            for (int i = 0; i < 8; i++) {
                long node_id = 8 * cell_id + i;
                node_ids[i] = node_id;
            }
        };

        auto get_cell_data = [&](int cell_id) { return field[cell_id]; };

        Parfait::vtk::Writer writer(
            std::move(filename), 8 * extents.size(), extents.size(), get_point, get_cell_type, get_cell_nodes);
        writer.addCellField("some-field", get_cell_data);
        writer.visualize();
    }
    inline void write(std::string filename,
                      const std::vector<Parfait::Extent<double>>& extents,
                      const std::vector<int>& field) {
        std::vector<double> double_field(field.size());
        for (size_t i = 0; i < double_field.size(); i++) double_field[i] = field[i];
        write(filename, extents, double_field);
    }
}
}
