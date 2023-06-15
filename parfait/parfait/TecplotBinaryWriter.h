#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Parfait {
namespace tecplot {
    class BinaryWriter : public Visualization::Writer {
      public:
        BinaryWriter(std::string filename,
                     long num_points,
                     long num_cells,
                     std::function<void(long, double*)> getPoint,
                     std::function<int(long)> getVTKCellType,
                     std::function<void(int, int*)> getCellNodes);

        virtual void addCellField(const std::string& name, std::function<double(int)> f) override;
        virtual void addNodeField(const std::string& name, std::function<double(int)> f) override;
        virtual void visualize() override;

        void setSolutionTime(double s);

      private:
        std::string filename;
        int num_points;
        int num_volume_cells;
        bool has_surface_zone;
        bool has_volume_zone;
        double solution_time = 0.0;
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
        void close();
        void writeFloat(float);
        void writeInt(int);
        void writeDouble(double d);
        void writeDoubles(double* d, int num_to_write);
        void writeData();
        void writeRanges();
        void writeNodes();
        void writeHeader();
        void writeFields();
        void writeCells();
        std::array<double, 2> getMinMaxPointDimension(int dimension);
        std::array<double, 2> getMinMax_nodeField(std::string name);

        std::vector<double> getPointDimension(int dimension);
        std::vector<double> getFieldAsVector_node(std::string name);
    };
}
}

#include "TecplotBinaryWriter.hpp"
