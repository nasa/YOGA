#pragma once
#include <parfait/Throw.h>

#include "ByteSwap.h"
#include "CellWindingConverters.h"
#include "Point.h"
#include <parfait/StringTools.h>
#include "VTKWriter.h"
#include "TecplotBrickCollapse.h"

namespace Parfait {
namespace tecplot {

    inline Writer::Writer(std::string filename,
                          long num_points,
                          long num_cells,
                          std::function<void(long, double*)> getPoint,
                          std::function<int(long)> getVTKCellType,
                          std::function<void(int, int*)> getCellNodes)
        : filename(std::move(filename)),
          getPoint(std::move(getPoint)),
          getCellType(std::move(getVTKCellType)),
          getCellNodes(std::move(getCellNodes)),
          fp(nullptr) {
        for (long c = 0; c < num_cells; c++) {
            using namespace Parfait::Visualization;
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
    }

    inline void Writer::addCellField(const std::string& name, std::function<double(int)> f) {
        printf("Writing file, but skipping cell field %s, only node data is supported\n", name.c_str());
    }
    inline void Writer::addNodeField(const std::string& name, std::function<double(int)> f) {
        node_fields[name] = std::move(f);
    }
    inline void Writer::visualize() {
        open();
        if (has_surface_zone) {
            fprintf(fp,
                    "ZONE T = \"Surface\", N = %ld, E = %ld, DATAPACKING = POINT, ZONETYPE = FEQUADRILATERAL\n",
                    surface_to_global_points.size(),
                    surface_to_global_cells.size());
            writePointsAndFields(surface_to_global_points);
            writeSurfaceCells();
        }

        if (has_volume_zone) {
            fprintf(fp,
                    "ZONE T = \"Volume\", N = %ld, E = %ld, DATAPACKING = POINT, ZONETYPE = FEBRICK\n",
                    volume_to_global_points.size(),
                    volume_to_global_cells.size());
            writePointsAndFields(volume_to_global_points);
            writeVolumeCells();
        }
        close();
    }
    inline void Writer::open() {
        if (Parfait::StringTools::getExtension(filename) != "tec") filename += ".tec";
        fp = fopen(filename.c_str(), "w");
        if (fp == nullptr) PARFAIT_THROW("Could not open file for writing: " + filename);
        fprintf(fp, "TITLE = \"Powered by T-infinity\"\n");
        fprintf(fp, R"(VARIABLES = "X", "Y", "Z")");
        for (auto& pair : node_fields) {
            auto name = pair.first;
            fprintf(fp, "\"%s\"", name.c_str());
        }
        fprintf(fp, "\n");
    }
    inline void Writer::writePointsAndFields(const std::vector<long>& compact_to_global_node_ids) {
        Parfait::Point<double> p;
        for (long global : compact_to_global_node_ids) {
            getPoint(global, p.data());
            fprintf(fp, "%e %e %e", p[0], p[1], p[2]);
            for (auto& pair : node_fields) {
                double d = pair.second(global);
                fprintf(fp, " %e", d);
            }
            fprintf(fp, "\n");
        }
    }
    inline void Writer::writeSurfaceCells() {
        fprintf(fp, "\n");
        std::array<int, 4> cell_nodes;
        for (long gcid : surface_to_global_cells) {
            auto type = getCellType(gcid);
            cell_nodes.fill(0);  // so we don't create map at errors mapping the 4th node in a triangle
            getCellNodes(gcid, cell_nodes.data());
            for (int i = 0; i < 4; i++) {
                auto gnid = cell_nodes[i];
                cell_nodes[i] = surface_global_to_local_points.at(gnid);
            }
            writeSurfaceCellNodes(type, cell_nodes);
        }
    }
    inline void Writer::writeVolumeCells() {
        fprintf(fp, "\n");
        std::array<int, 8> cell_nodes;
        for (long gcid : volume_to_global_cells) {
            auto type = getCellType(gcid);
            cell_nodes.fill(0);  // so we don't create map at errors mapping the 5th node in a tet
            getCellNodes(gcid, cell_nodes.data());
            for (int i = 0; i < 8; i++) {
                auto gnid = cell_nodes[i];
                cell_nodes[i] = volume_global_to_local_points.at(gnid);
            }
            writeVolumeCellNodes(type, cell_nodes);
        }
    }
    inline void Writer::close() {
        if (fp) {
            fclose(fp);
            fp = nullptr;
        }
    }
    inline void Writer::writeVolumeCellNodes(int type, std::array<int, 8> cell_nodes) {
        for (auto& n : cell_nodes) {
            n++;
        }
        cell_nodes = collapseCellToBrick(type, cell_nodes);
        fprintf(fp,
                "%d %d %d %d %d %d %d %d\n",
                cell_nodes[0],
                cell_nodes[1],
                cell_nodes[2],
                cell_nodes[3],
                cell_nodes[4],
                cell_nodes[5],
                cell_nodes[6],
                cell_nodes[7]);
    }

    inline void Writer::writeSurfaceCellNodes(int type, std::array<int, 4>& cell_nodes) {
        using namespace Parfait::Visualization;
        for (auto& n : cell_nodes) {
            n++;
        }
        switch (type) {
            case CellType::TRI:
                fprintf(fp, "%d %d %d %d\n", cell_nodes[0], cell_nodes[1], cell_nodes[2], cell_nodes[2]);
                break;
            case CellType::QUAD:
                fprintf(fp, "%d %d %d %d\n", cell_nodes[0], cell_nodes[1], cell_nodes[2], cell_nodes[3]);
                break;
            default:
                PARFAIT_WARNING("Unknown cell type found.  Writing a degenerate cell");
                fprintf(fp, "%d %d %d %d\n", cell_nodes[0], cell_nodes[1], cell_nodes[2], cell_nodes[3]);
        }
    }
}
}
