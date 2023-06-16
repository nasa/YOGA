#pragma once

namespace Parfait {
namespace vtk {
    inline int getNumPointsFromVTKType(int vtk_type) {
        switch (vtk_type) {
            case 1:
                return 1;  // node
            case 3:
                return 2;  // edge
            case 5:
                return 3;  // triangle
            case 9:
                return 4;  // quad
            case 10:
                return 4;  // tet
            case 12:
                return 8;  // hex
            case 13:
                return 6;  // prism
            case 14:
                return 5;  // pyramid
            case 22:
                return 6;  // quadratic triangle
            case 24:
                return 10;  // quadratic triangle
            case 32:
                return 18;  // quadratic triangle
            default:
                PARFAIT_THROW("Need to map additional number of nodes for vtk type: " + std::to_string(vtk_type));
        }
    }
}
}
