#pragma once
#include <array>
#include <functional>
#include <limits>
#include <parfait/Throw.h>
#include "ByteSwap.h"
#include "CellWindingConverters.h"
#include "Point.h"
#include <parfait/StringTools.h>
#include "VTKWriter.h"
#include "TecplotBinaryWriter.h"
#include "TecplotBinaryHelpers.h"

namespace Parfait {
namespace tecplot {

    inline BinaryWriter::BinaryWriter(std::string filename,
                                      long num_points,
                                      long num_cells,
                                      std::function<void(long, double*)> getPoint,
                                      std::function<int(long)> getVTKCellType,
                                      std::function<void(int, int*)> getCellNodes)
        : filename(std::move(filename)),
          num_points(int(num_points)),  // yes, tecpclot only supports 32 bit num points
          getPoint(std::move(getPoint)),
          getCellType(std::move(getVTKCellType)),
          getCellNodes(std::move(getCellNodes)),
          fp(nullptr) {
        using namespace Parfait::Visualization;
        for (long c = 0; c < num_cells; c++) {
            auto type = getCellType(c);
            switch (type) {
                case NODE:
                case EDGE:
                    break;
                case TRI:
                case QUAD:
                    has_surface_zone = true;
                    surface_to_global_cells.push_back(c);
                    break;
                case TET:
                case PYRAMID:
                case PRISM:
                case HEX:
                    has_volume_zone = true;
                    volume_to_global_cells.push_back(c);
                    break;
                default:
                    PARFAIT_THROW("UNEXPECTED cell type encountered: " + std::to_string(type));
            }
        }

        std::vector<int> cell_nodes;
        cell_nodes.reserve(8);

        std::set<long> already_added;
        for (auto gcid : surface_to_global_cells) {
            auto type = getCellType(gcid);
            auto length = Parfait::vtk::getNumPointsFromVTKType(type);
            cell_nodes.resize(length);
            this->getCellNodes(gcid, cell_nodes.data());

            for (auto gnid : cell_nodes) {
                if (already_added.count(gnid) == 0) {
                    already_added.insert(gnid);
                    surface_global_to_local_points[gnid] = surface_to_global_points.size();
                    surface_to_global_points.push_back(gnid);
                }
            }
        }

        already_added.clear();
        for (auto gcid : volume_to_global_cells) {
            auto type = getCellType(gcid);
            auto length = Parfait::vtk::getNumPointsFromVTKType(type);
            cell_nodes.resize(length);
            this->getCellNodes(gcid, cell_nodes.data());

            for (auto gnid : cell_nodes) {
                if (already_added.count(gnid) == 0) {
                    already_added.insert(gnid);
                    volume_global_to_local_points[gnid] = volume_to_global_points.size();
                    volume_to_global_points.push_back(gnid);
                }
            }
        }

        num_volume_cells = int(volume_to_global_cells.size());
    }

    inline void BinaryWriter::addCellField(const std::string& name, std::function<double(int)> f) {
        PARFAIT_WARNING(
            "Writer can't handle cell fields yet.\nContact a T-infinity dev, if you need this feature "
            "we'll be happy to add it.\nMatthew.D.OConnell@nasa.gov\nCameron.T.Druyor@nasa.gov");
    }
    inline void BinaryWriter::addNodeField(const std::string& name, std::function<double(int)> f) {
        node_fields[name] = std::move(f);
    }
    inline void BinaryWriter::visualize() {
        open();
        writeHeader();
        writeData();
        close();
    }

    inline void BinaryWriter::writeHeader() {
        using namespace Parfait::tecplot::binary_helpers;

        ZoneHeaderInfo zone_header_info;
        zone_header_info.type = ZoneHeaderInfo::VOLUME;

        int num_variables = 3 + node_fields.size() + cell_fields.size();
        std::vector<std::string> variable_names;
        std::vector<Parfait::tecplot::VariableLocation> variable_locations;
        for (auto& key_value : node_fields) {
            variable_names.push_back(key_value.first);
            variable_locations.push_back(Parfait::tecplot::NODE);
        }
        for (auto& key_value : cell_fields) {
            variable_names.push_back(key_value.first);
            variable_locations.push_back(Parfait::tecplot::CELL);
        }
        int num_elements = int(volume_to_global_cells.size());
        zone_header_info.num_points = num_points;
        zone_header_info.num_elements = num_elements;
        binary_helpers::writeHeader(
            fp, num_variables, variable_names, variable_locations, {zone_header_info}, solution_time);
    }
    inline void BinaryWriter::open() {
        if (Parfait::StringTools::getExtension(filename) != "plt") filename += ".plt";
        fp = fopen(filename.c_str(), "w");
        if (fp == nullptr) PARFAIT_THROW("Could not open file for writing: " + filename);
    }
    inline void BinaryWriter::close() {
        if (fp) {
            fclose(fp);
            fp = nullptr;
        }
    }
    inline void BinaryWriter::writeRanges() {
        std::array<double, 2> min_max_X = getMinMaxPointDimension(0);
        std::array<double, 2> min_max_Y = getMinMaxPointDimension(1);
        std::array<double, 2> min_max_Z = getMinMaxPointDimension(2);
        writeDoubles(min_max_X.data(), 2);
        writeDoubles(min_max_Y.data(), 2);
        writeDoubles(min_max_Z.data(), 2);

        for (auto& key_value : node_fields) {
            auto min_max = getMinMax_nodeField(key_value.first);
            writeDoubles(min_max.data(), 2);
        }
    }
    inline void BinaryWriter::writeNodes() {
        for (int i = 0; i < 3; i++) {
            auto x = getPointDimension(i);
            writeDoubles(x.data(), num_points);
        }
    }
    inline void BinaryWriter::writeFields() {
        for (auto& key_value : node_fields) {
            auto f = getFieldAsVector_node(key_value.first);
            writeDoubles(f.data(), num_points);
        }
    }
    inline void BinaryWriter::writeFloat(float f) { fwrite(&f, sizeof(float), 1, fp); }
    inline void BinaryWriter::writeInt(int i) { fwrite(&i, sizeof(int), 1, fp); }
    inline void BinaryWriter::writeDoubles(double* d, int num_to_write) { fwrite(d, sizeof(double), num_to_write, fp); }
    inline void BinaryWriter::writeData() {
        writeFloat(299.0);
        for (int i = 0; i < int(node_fields.size() + 3); i++) {
            writeInt(2);  // we're going to write all fields as doubles
        }

        writeInt(0);   // no passive variables (whatever those are)
        writeInt(0);   // no variable sharing (whatever that is)
        writeInt(-1);  // no zone sharing (whatever THAT is)

        writeRanges();
        writeNodes();
        writeFields();
        writeCells();
    }

    inline void BinaryWriter::writeCells() {
        std::array<int, 8> cell_nodes;
        for (long gcid : volume_to_global_cells) {
            auto type = getCellType(gcid);
            cell_nodes.fill(0);  // so we don't create map at errors mapping the 5th node in a tet
            getCellNodes(gcid, cell_nodes.data());
            for (int i = 0; i < 8; i++) {
                auto gnid = cell_nodes[i];
                cell_nodes[i] = volume_global_to_local_points.at(gnid);
            }
            auto cell_as_brick = collapseCellToBrick(type, cell_nodes);
            fwrite(cell_as_brick.data(), sizeof(int), 8, fp);
        }
    }

    inline std::vector<double> BinaryWriter::getFieldAsVector_node(std::string name) {
        auto& field = node_fields.at(name);
        std::vector<double> x(volume_to_global_points.size());
        for (int i = 0; i < int(volume_to_global_points.size()); i++) {
            int gid = volume_to_global_points[i];
            x[i] = field(gid);
        }
        return x;
    }

    inline std::vector<double> BinaryWriter::getPointDimension(int d) {
        std::vector<double> x(volume_to_global_points.size());
        for (int i = 0; i < int(volume_to_global_points.size()); i++) {
            int gid = volume_to_global_points[i];
            std::array<double, 3> p;
            getPoint(gid, p.data());
            x[i] = p[d];
        }
        return x;
    }

    inline std::array<double, 2> BinaryWriter::getMinMaxPointDimension(int d) {
        std::array<double, 2> min_max = {std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
        for (int i = 0; i < num_points; i++) {
            Parfait::Point<double> p;
            getPoint(i, p.data());
            min_max[0] = std::min(p[d], min_max[0]);
            min_max[1] = std::max(p[d], min_max[1]);
        }
        return min_max;
    }
    inline std::array<double, 2> BinaryWriter::getMinMax_nodeField(std::string name) {
        std::array<double, 2> min_max = {std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
        auto& field = node_fields.at(name);
        for (int i = 0; i < num_points; i++) {
            double d = field(i);
            min_max[0] = std::min(d, min_max[0]);
            min_max[1] = std::max(d, min_max[1]);
        }
        return min_max;
    }
    inline void BinaryWriter::setSolutionTime(double s) { solution_time = s; }
}
}
