#pragma once
#include <string>
#include <map>
#include <vector>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/FieldInterface.h>
#include <parfait/StringTools.h>
#include <parfait/Throw.h>
#include <parfait/Point.h>
#include <parfait/MapTools.h>
#include <parfait/DataFrame.h>
#include <parfait/LinePlotters.h>
#include <t-infinity/FieldTools.h>

class ParallelDataFrameWriter {
  public:
    inline ParallelDataFrameWriter(std::string filename_in,
                                   std::shared_ptr<inf::MeshInterface> mesh_in,
                                   MessagePasser mp_in)
        : filename(filename_in),
          mp(mp_in),
          mesh(mesh_in),
          global_node_count(inf::globalNodeCount(mp, *mesh)),
          global_cell_count(inf::globalCellCount(mp, *mesh)) {}

    inline void addField(std::shared_ptr<inf::FieldInterface> f) {
        if (f->association() == inf::FieldAttributes::Node()) node_fields[f->name()] = f;
        if (f->association() == inf::FieldAttributes::Cell()) cell_fields[f->name()] = f;
    }
    inline void write() {
        auto extension = Parfait::StringTools::getExtension(filename);
        writeNodes();
        writeCells();
    }

    inline void spatiallySort(Parfait::DataFrame& df) {
        if (df.has("x")) {
            double min = df.min("x");
            double max = df.max("x");
            double range = max - min;
            if (range > 1e-8) {
                df.sort("x");
                return;
            }
        }
        if (df.has("y")) {
            double min = df.min("y");
            double max = df.max("y");
            double range = max - min;
            if (range > 1e-8) {
                df.sort("y");
                return;
            }
        }
        if (df.has("z")) {
            double min = df.min("z");
            double max = df.max("z");
            double range = max - min;
            if (range > 1e-8) {
                df.sort("z");
                return;
            }
        }
    }

    inline void writeCells() {
        if (cell_fields.size() == 0) return;
        auto output_filename = filename;
        auto extension = Parfait::StringTools::getExtension(output_filename);
        output_filename = Parfait::StringTools::stripExtension(output_filename);
        output_filename += "_cells." + extension;

        std::map<std::string, std::vector<double>> all_output_fields_gathered = gatherAllOutputFields(cell_fields);
        std::vector<double> x, y, z;
        gatherCellXYZ(x, y, z);
        all_output_fields_gathered["x"] = x;
        all_output_fields_gathered["y"] = y;
        all_output_fields_gathered["z"] = z;
        if (mp.Rank() == 0) {
            Parfait::DataFrame data_frame(all_output_fields_gathered);
            spatiallySort(data_frame);
            if (extension == "dat") {
                Parfait::writeTecplot(output_filename, data_frame);
            } else if (extension == "csv") {
                Parfait::writeCSV(output_filename, data_frame);
            }
        }
    }

    inline void writeNodes() {
        if (node_fields.size() == 0) return;

        auto output_filename = filename;
        auto extension = Parfait::StringTools::getExtension(output_filename);
        output_filename = Parfait::StringTools::stripExtension(output_filename);
        output_filename += "_nodes." + extension;

        std::map<std::string, std::vector<double>> all_output_fields_gathered = gatherAllOutputFields(node_fields);
        std::vector<double> x, y, z;
        gatherNodeXYZ(x, y, z);
        all_output_fields_gathered["x"] = x;
        all_output_fields_gathered["y"] = y;
        all_output_fields_gathered["z"] = z;
        if (mp.Rank() == 0) {
            Parfait::DataFrame data_frame(all_output_fields_gathered);
            spatiallySort(data_frame);
            if (extension == "dat") {
                Parfait::writeTecplot(output_filename, data_frame);
            } else if (extension == "csv") {
                Parfait::writeCSV(output_filename, data_frame);
            }
        }
    }

  private:
    std::string filename;
    MessagePasser mp;
    std::shared_ptr<inf::MeshInterface> mesh;
    long global_node_count;
    long global_cell_count;

    std::map<std::string, std::shared_ptr<inf::FieldInterface>> node_fields;
    std::map<std::string, std::shared_ptr<inf::FieldInterface>> cell_fields;

    inline std::map<std::string, std::vector<double>> gatherAllOutputFields(
        const std::map<std::string, std::shared_ptr<inf::FieldInterface>> fields) {
        std::map<std::string, std::vector<double>> output_fields;
        for (auto& pair : fields) {
            auto& f = pair.second;
            output_fields[f->name()] = inf::FieldTools::gatherScalarField(mp, *mesh, *f, 0);
        }
        return output_fields;
    }

    inline void gatherNodeXYZ(std::vector<double>& x, std::vector<double>& y, std::vector<double>& z) {
        std::map<long, Parfait::Point<double>> my_points;
        for (int n = 0; n < mesh->nodeCount(); n++) {
            if (mesh->nodeOwner(n) == mp.Rank()) {
                auto p = mesh->node(n);
                auto global = mesh->globalNodeId(n);
                my_points[global] = p;
            }
        }

        auto all_points = mp.GatherByOrdinalRange(my_points, long(0), global_node_count, 0);

        x.resize(all_points.size());
        y.resize(all_points.size());
        z.resize(all_points.size());

        for (long i = 0; i < long(all_points.size()); i++) {
            x[i] = all_points[i][0];
            y[i] = all_points[i][1];
            z[i] = all_points[i][2];
        }
    }

    inline void gatherCellXYZ(std::vector<double>& x, std::vector<double>& y, std::vector<double>& z) {
        std::map<long, Parfait::Point<double>> my_points;
        for (int c = 0; c < mesh->cellCount(); c++) {
            if (mesh->cellOwner(c) == mp.Rank()) {
                auto cell = inf::Cell(mesh, c);
                auto p = cell.averageCenter();
                auto global = mesh->globalCellId(c);
                my_points[global] = p;
            }
        }

        auto all_points = mp.GatherByOrdinalRange(my_points, long(0), global_cell_count, 0);

        x.resize(all_points.size());
        y.resize(all_points.size());
        z.resize(all_points.size());

        for (long i = 0; i < long(all_points.size()); i++) {
            x[i] = all_points[i][0];
            y[i] = all_points[i][1];
            z[i] = all_points[i][2];
        }
    }
};
