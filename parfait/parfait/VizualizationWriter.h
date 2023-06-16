#pragma once
#include <functional>
#include <string>

namespace Parfait {
namespace Visualization {
    enum CellType {
        NODE = 1,
        EDGE = 3,
        TRI = 5,
        QUAD = 9,
        TET = 10,
        PYRAMID = 14,
        PRISM = 13,
        HEX = 12,
        TRI_P2 = 22,
        TET_P2 = 24,
        PENTA_18 = 32
    };
    class Writer {
      public:
        virtual ~Writer() = default;
        virtual void addCellField(const std::string& name, std::function<double(int)> f) = 0;
        virtual void addNodeField(const std::string& name, std::function<double(int)> f) = 0;
        virtual void visualize() = 0;
    };
}
}
