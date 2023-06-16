#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>

namespace inf {

class MeshInterface {
  public:
    enum CellType {  //--- Following the CGNS element winding convention.
        NODE = 2,
        BAR_2 = 3,
        BAR_3 = 4,
        BAR_4 = 24,
        BAR_5 = 40,
        TRI_3 = 5,
        TRI_6 = 6,
        TRI_9 = 25,
        TRI_10 = 26,
        TRI_12 = 41,
        TRI_15 = 42,
        QUAD_4 = 7,
        QUAD_8 = 8,
        QUAD_9 = 9,
        QUAD_12 = 27,
        QUAD_16 = 28,
        QUAD_P4_16 = 43,
        QUAD_25 = 44,
        TETRA_4 = 10,
        TETRA_10 = 11,
        TETRA_16 = 29,
        TETRA_20 = 30,
        TETRA_22 = 45,
        TETRA_34 = 46,
        TETRA_35 = 47,
        PYRA_5 = 12,
        PYRA_14 = 13,
        PYRA_13 = 21,
        PYRA_21 = 31,
        PYRA_29 = 32,
        PYRA_30 = 33,
        PYRA_P4_29 = 48,
        PYRA_50 = 49,
        PYRA_55 = 50,
        PENTA_6 = 14,
        PENTA_15 = 15,
        PENTA_18 = 16,
        PENTA_24 = 34,
        PENTA_38 = 35,
        PENTA_40 = 36,
        PENTA_33 = 51,
        PENTA_66 = 52,
        PENTA_75 = 53,
        HEXA_8 = 17,
        HEXA_20 = 18,
        HEXA_27 = 19,
        HEXA_32 = 37,
        HEXA_56 = 38,
        HEXA_64 = 39,
        HEXA_44 = 54,
        HEXA_98 = 55,
        HEXA_125 = 56,
        PIXEL = 100001,
        VOXEL = 200002,
        NULL_CELL = 101325
    };
    virtual ~MeshInterface() = default;
    virtual int nodeCount() const = 0;
    virtual int cellCount() const = 0;
    virtual int cellCount(CellType cell_type) const = 0;
    virtual int partitionId() const = 0;

    inline int countOwnedCells() {
        int cell_count = cellCount();
        int count = 0;
        for (int c = 0; c < cell_count; c++) {
            if (cellOwner(c) == partitionId()) count++;
        }
        return count;
    }

    virtual void nodeCoordinate(int node_id, double* coord) const = 0;
    inline std::array<double, 3> node(int node_id) const {
        std::array<double, 3> p;
        nodeCoordinate(node_id, p.data());
        return p;
    }

    virtual CellType cellType(int cell_id) const = 0;
    virtual void cell(int cell_id, int* cell_out) const = 0;
    void cell(int cell_id, std::vector<int>& cell_nodes) const {
        auto length = cellTypeLength(cellType(cell_id));
        cell_nodes.resize(length);
        cell(cell_id, cell_nodes.data());
    }
    std::vector<int> cell(int cell_id) const {
        auto length = cellTypeLength(cellType(cell_id));
        std::vector<int> cell_nodes(length);
        cell(cell_id, cell_nodes.data());
        return cell_nodes;
    }

    virtual long globalNodeId(int node_id) const = 0;
    virtual int cellTag(int cell_id) const = 0;
    virtual int nodeOwner(int node_id) const = 0;

    virtual long globalCellId(int cell_id) const = 0;
    virtual int cellOwner(int cell_id) const = 0;

    inline int cellLength(int cell_id) const {
        auto type = cellType(cell_id);
        return cellTypeLength(type);
    }

    inline bool ownedNode(int node_id) const { return nodeOwner(node_id) == partitionId(); }

    inline bool ownedCell(int cell_id) const { return cellOwner(cell_id) == partitionId(); }

    static inline int cellTypeDimensionality(CellType cell_type) {
        switch (cell_type) {
            case NODE:
                return 0;
            case BAR_2:
            case BAR_3:
            case BAR_4:
            case BAR_5:
                return 1;
            case TRI_3:
            case TRI_6:
            case TRI_9:
            case TRI_10:
            case TRI_12:
            case TRI_15:
            case QUAD_4:
            case QUAD_8:
            case QUAD_9:
            case QUAD_12:
            case QUAD_16:
            case QUAD_P4_16:
            case QUAD_25:
                return 2;
            case TETRA_4:
            case TETRA_10:
            case TETRA_16:
            case TETRA_20:
            case TETRA_22:
            case TETRA_34:
            case TETRA_35:
            case PYRA_5:
            case PYRA_14:
            case PYRA_13:
            case PYRA_21:
            case PYRA_29:
            case PYRA_30:
            case PYRA_P4_29:
            case PYRA_50:
            case PYRA_55:
            case PENTA_6:
            case PENTA_15:
            case PENTA_18:
            case PENTA_24:
            case PENTA_38:
            case PENTA_40:
            case PENTA_33:
            case PENTA_66:
            case PENTA_75:
            case HEXA_8:
            case HEXA_20:
            case HEXA_27:
            case HEXA_32:
            case HEXA_56:
            case HEXA_64:
            case HEXA_44:
            case HEXA_98:
            case HEXA_125:
                return 3;
            default:
                throw std::logic_error("MeshInterface: dimensionality of type not supported: " +
                                       std::to_string(cell_type));
        }
    }

    static inline int cellTypeLength(CellType cell_type) {
        switch (cell_type) {
            case MeshInterface::CellType::NODE:
                return 1;
            case MeshInterface::CellType::BAR_2:
                return 2;
            case MeshInterface::CellType::TRI_3:
                return 3;
            case MeshInterface::CellType::TRI_6:
                return 6;
            case MeshInterface::CellType::TRI_10:
                return 10;
            case MeshInterface::CellType::TRI_15:
                return 15;
            case MeshInterface::CellType::QUAD_4:
                return 4;
            case MeshInterface::CellType::QUAD_8:
                return 8;
            case MeshInterface::CellType::QUAD_9:
                return 9;
            case MeshInterface::CellType::TETRA_4:
                return 4;
            case MeshInterface::CellType::TETRA_10:
                return 10;
            case MeshInterface::CellType::TETRA_20:
                return 20;
            case MeshInterface::CellType::TETRA_35:
                return 35;
            case MeshInterface::CellType::PENTA_6:
                return 6;
            case MeshInterface::CellType::PENTA_15:
                return 15;
            case MeshInterface::CellType::PENTA_18:
                return 18;
            case MeshInterface::CellType::PENTA_40:
                return 40;
            case MeshInterface::CellType::PENTA_75:
                return 75;
            case MeshInterface::CellType::PYRA_5:
                return 5;
            case MeshInterface::CellType::PYRA_13:
                return 13;
            case MeshInterface::CellType::PYRA_14:
                return 14;
            case MeshInterface::CellType::HEXA_8:
                return 8;
            case MeshInterface::CellType::HEXA_20:
                return 20;
            case MeshInterface::CellType::HEXA_27:
                return 27;
            default:
                throw std::logic_error("MeshInterface: length of type not supported: " +
                                       std::to_string(cell_type));
        }
    }

    static inline std::string cellTypeString(CellType cell_type) {
        switch (cell_type) {
            case NODE:
                return "NODE";
            case BAR_2:
                return "BAR_2";
            case BAR_3:
                return "BAR_3";
            case TRI_3:
                return "TRI_3";
            case TRI_6:
                return "TRI_6";
            case TRI_9:
                return "TRI_9";
            case TRI_10:
                return "TRI_10";
            case TRI_15:
                return "TRI_15";
            case QUAD_4:
                return "QUAD_4";
            case QUAD_8:
                return "QUAD_8";
            case QUAD_9:
                return "QUAD_9";
            case QUAD_12:
                return "QUAD_12";
            case QUAD_16:
                return "QUAD_16";
            case TETRA_4:
                return "TETRA_4";
            case TETRA_10:
                return "TETRA_10";
            case TETRA_16:
                return "TETRA_16";
            case TETRA_20:
                return "TETRA_20";
            case TETRA_35:
                return "TETRA_35";
            case PYRA_5:
                return "PYRA_5";
            case PYRA_13:
                return "PYRA_13";
            case PYRA_14:
                return "PYRA_14";
            case PENTA_6:
                return "PENTA_6";
            case PENTA_15:
                return "PENTA_15";
            case PENTA_18:
                return "PENTA_18";
            case PENTA_40:
                return "PENTA_40";
            case PENTA_75:
                return "PENTA_75";
            case HEXA_8:
                return "HEXA_8";
            case HEXA_20:
                return "HEXA_20";
            case HEXA_27:
                return "HEXA_27";
            default:
                throw std::logic_error("MeshInterface: have not implemented string of this type: " +
                                       std::to_string(int(cell_type)));
        }
    }

    static inline bool is3DCellType(CellType cell_type) {
        return cellTypeDimensionality(cell_type) == 3;
    }
    static inline bool is2DCellType(CellType cell_type) {
        return cellTypeDimensionality(cell_type) == 2;
    }
    static inline bool is1DCellType(CellType cell_type) {
        return cellTypeDimensionality(cell_type) == 1;
    }
    inline bool is3DCell(int cell_id) const { return is3DCellType(cellType(cell_id)); }
    inline bool is2DCell(int cell_id) const { return is2DCellType(cellType(cell_id)); }
    inline bool is1DCell(int cell_id) const { return is1DCellType(cellType(cell_id)); }
    inline int cellDimensionality(int cell_id) const {
        return cellTypeDimensionality(cellType(cell_id));
    }

    inline virtual std::string tagName(int tag) const {
        return std::string("Tag_") + std::to_string(tag);
    }

    inline virtual std::string type() const { return typeid(*this).name(); }
};
}
