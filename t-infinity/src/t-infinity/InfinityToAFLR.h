#pragma once
#include <parfait/CellWindingConverters.h>
#include "MeshInterface.h"

namespace inf {
namespace toAFLR {
    template <typename T>
    void rewindCell(MeshInterface::CellType type, T* cell) {
        switch (type) {
            case MeshInterface::TRI_3:
            case MeshInterface::QUAD_4:
                return;
            case MeshInterface::TETRA_4:
                Parfait::CGNSToAflr::convertTet(cell);
                return;
            case MeshInterface::PYRA_5:
                Parfait::CGNSToAflr::convertPyramid(cell);
                return;
            case MeshInterface::PENTA_6:
                Parfait::CGNSToAflr::convertPrism(cell);
                return;
            case MeshInterface::HEXA_8:
                Parfait::CGNSToAflr::convertHex(cell);
                return;
            default:
                PARFAIT_WARNING("Cell type " + MeshInterface::cellTypeString(type) +
                                " not supported by AFLR");
        }
    }
}
}
