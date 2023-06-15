
#pragma once
#include <parfait/CellWindingConverters.h>
#include <parfait/Point.h>
#include <parfait/Throw.h>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <parfait/StringTools.h>
#include "VizualizationWriter.h"

namespace Parfait {
namespace vtk {
    class Writer : public Visualization::Writer {
      public:
        enum CellType {
            NODE = 1,
            EDGE = 3,
            TRI = 5,
            QUAD = 9,
            QUAD_8 = 23,
            TET = 10,
            PYRAMID = 14,
            PRISM = 13,
            HEX = 12,
            HEX_P2 = 25,
            TRI_P2 = 22,
            TET_P2 = 24,
            PENTA_18 = 32
        };
        Writer(std::string filename,
               long num_points,
               long num_cells,
               std::function<void(long, double*)> getPoint,
               std::function<CellType(int)> getVTKCellType,
               std::function<void(int, int*)> getCellNodes);

        virtual void addCellField(const std::string& name, std::function<double(int)> f) override;
        virtual void addCellTensorField(const std::string& name, std::function<void(int, double*)> f);
        virtual void addNodeField(const std::string& name, std::function<double(int)> f) override;
        virtual void addNodeTensorField(const std::string& name, std::function<void(int, double*)> f);
        virtual void visualize() override;

        static void writeHeader(FILE*& f);
        static std::string getHeader();
        static void rewindCellNodes(CellType type, std::vector<int>& cell_nodes);

      private:
        std::string filename;
        long num_points;
        long num_cells;
        std::function<void(int, double*)> getPoint;
        std::function<CellType(int)> getVTKCellType;
        std::function<void(int, int*)> getCellNodes;
        std::map<std::string, std::function<double(int)>> cell_scalars;
        std::map<std::string, std::function<void(int, double*)>> cell_tensors;
        std::map<std::string, std::function<double(int)>> node_scalars;
        std::map<std::string, std::function<void(int, double*)>> node_tensors;
        FILE* fp;

        void writePoints();
        size_t calcCellBufferSize();
        void open();
        void writeCells();
        void writeCellField(std::string field_name, std::function<double(int)> field);
        void writeCellField(std::string field_name, std::function<void(int, double*)> field);
        void writeNodeField(std::string field_name, std::function<double(int)> field);
        void writeNodeField(std::string field_name, std::function<void(int, double*)> field);
        void close();
        void writeNodeFields();
        void writeCellFields();
    };
}
}

#include "VTKWriter.hpp"
