#pragma once
#include <string>
#include <functional>
#include <map>
#include <vector>
#include "VizualizationWriter.h"

namespace Parfait {
namespace tecplot {
    class Writer : public Visualization::Writer {
      public:
        Writer(std::string filename,
               long num_points,
               long num_cells,
               std::function<void(long, double*)> getPoint,
               std::function<int(long)> getVTKCellType,
               std::function<void(int, int*)> getCellNodes);

        virtual void addCellField(const std::string& name, std::function<double(int)> f) override;
        virtual void addNodeField(const std::string& name, std::function<double(int)> f) override;
        virtual void visualize() override;

      private:
        std::string filename;
        bool has_surface_zone = false;
        bool has_volume_zone = false;
        std::vector<long> surface_to_global_cells;
        std::vector<long> volume_to_global_cells;
        std::vector<long> surface_to_global_points;
        std::vector<long> volume_to_global_points;
        std::map<long, long> surface_global_to_local_points;
        std::map<long, long> volume_global_to_local_points;
        std::function<void(int, double*)> getPoint;
        std::function<int(int)> getCellType;
        std::function<void(int, int*)> getCellNodes;
        std::map<std::string, std::function<double(int)>> cell_fields;
        std::map<std::string, std::function<double(int)>> node_fields;
        FILE* fp;

        void open();
        void writePointsAndFields(const std::vector<long>& compact_to_global_node_ids);
        void writeSurfaceCells();
        void writeVolumeCells();
        void close();
        void writeSurfaceCellNodes(int type, std::array<int, 4>& cell_nodes);
        void writeVolumeCellNodes(int type, std::array<int, 8> cell_nodes);
    };
}
}

#include "TecplotWriter.hpp"