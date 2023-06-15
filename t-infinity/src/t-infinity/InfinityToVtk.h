#pragma once
#include <parfait/Throw.h>
#include <t-infinity/MeshInterface.h>

namespace inf {
inline int infinityToVtkCellType(inf::MeshInterface::CellType type) {
    using namespace inf;
    switch (type) {
        case MeshInterface::NODE:
            return 1;
        case MeshInterface::BAR_2:
            return 3;
        case MeshInterface::TRI_3:
            return 5;
        case MeshInterface::TRI_6:
            return 22;
        case MeshInterface::QUAD_4:
            return 9;
        case MeshInterface::QUAD_8:
            return 23;
        case MeshInterface::QUAD_9:
            return infinityToVtkCellType(MeshInterface::QUAD_8);
        case MeshInterface::TETRA_4:
            return 10;
        case MeshInterface::TETRA_10:
            return 24;
        case MeshInterface::PYRA_5:
            return 14;
        case MeshInterface::PYRA_13:
            return 27;
        case MeshInterface::PYRA_14:
            return infinityToVtkCellType(MeshInterface::PYRA_13);
        case MeshInterface::PENTA_6:
            return 13;
        case MeshInterface::PENTA_15:
            return 26;
        case MeshInterface::PENTA_18:
            return infinityToVtkCellType(MeshInterface::PENTA_15);
        case MeshInterface::HEXA_8:
            return 12;
        case MeshInterface::HEXA_20:
            return 25;
        case MeshInterface::HEXA_27:
            return infinityToVtkCellType(MeshInterface::HEXA_20);
        default:
            throw std::logic_error("VTK type related to mesh type not supported. TYPE: " +
                                   MeshInterface::cellTypeString(type));
    }
}

inline inf::MeshInterface::CellType vtkToInfinityCellType(int type) {
    using namespace inf;
    switch (type) {
        case 1:
            return MeshInterface::NODE;
        case 3:
            return MeshInterface::BAR_2;
        case 5:
            return MeshInterface::TRI_3;
        case 22:
            return MeshInterface::TRI_6;
        case 9:
            return MeshInterface::QUAD_4;
        case 23:
            return MeshInterface::QUAD_8;
        case 10:
            return MeshInterface::TETRA_4;
        case 24:
            return MeshInterface::TETRA_10;
        case 14:
            return MeshInterface::PYRA_5;
        case 13:
            return MeshInterface::PENTA_6;
        case 26:
            return MeshInterface::PENTA_15;
        case 32:
            return MeshInterface::PENTA_18;
        case 12:
            return MeshInterface::HEXA_8;
        default:
            PARFAIT_THROW("VTK type related to mesh type not supported. vtk type: " +
                          std::to_string(type));
    }
}
}
