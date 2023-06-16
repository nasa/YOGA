#pragma once
#include <parfait/Throw.h>
#include <t-infinity/FieldTools.h>
#include <parfait/PointWriter.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/ElevateMesh.h>
#include <t-infinity/Shortcuts.h>
#include <TECIO.h>
#include "parfait/MapTools.h"

namespace inf {
enum fileType_e { FULL = 0, GRID = 1, SOLUTION = 2 };

#define TEC_CHECK(i) PARFAIT_ASSERT(i == 0, "error with tecio call");

class SzWriter {
  public:
    enum TECIO_BASE_TYPES { BAR = 0, TRI = 1, QUAD = 2, TET = 3, HEX = 4, PYRAMID = 5, PENTA = 6 };
    enum TECIO_ORDER { P2 = 2 };
    enum BASIS_FUNCTIONS { DEFAULT = 0 };
    SzWriter(std::string filename, const inf::TinfMesh& m) : mesh(m), filename_base(filename) {
        //
    }

    void addNodeField(std::shared_ptr<inf::FieldInterface> f) { node_fields.push_back(f); }
    void addCellField(std::shared_ptr<inf::FieldInterface> f) { cell_fields.push_back(f); }

    void write() {
        auto types = Parfait::MapTools::keys(mesh.mesh.cell_tags);
        std::set<inf::MeshInterface::CellType> types_3D;
        std::set<inf::MeshInterface::CellType> types_2D;
        std::set<inf::MeshInterface::CellType> types_1D;
        for (auto t : types) {
            auto d = inf::MeshInterface::cellTypeDimensionality(t);
            switch (d) {
                case 0:
                    break;
                case 1:
                    types_1D.insert(t);
                    break;
                case 2:
                    types_2D.insert(t);
                    break;
                case 3:
                    types_3D.insert(t);
                    break;
                default:
                    PARFAIT_THROW("Invalid type found: " + std::to_string(d));
            }
        }
        openFile();
        writeZone("zone-1D", types_1D);
        writeZone("zone-2D", types_2D);
        writeZone("zone-3D", types_3D);
        closeFile();
    }

  private:
    const inf::TinfMesh& mesh;
    std::string filename_base;
    void* file_handle = nullptr;
    int next_field_index = -1;

    std::vector<std::shared_ptr<inf::FieldInterface>> node_fields;
    std::vector<std::shared_ptr<inf::FieldInterface>> cell_fields;

    int32_t use_shared_connectivity = 0;
    int32_t some_other_feature = 0;
    int32_t nodesAreOneBased = 0;
    int NODE_LOCATION = 1;
    int CELL_LOCATION = 0;

    std::string createSpaceDelimitedFieldNameNoSpaces() {
        std::string field_names = "x y z";
        for (auto f : node_fields) {
            auto name = f->name();
            name = Parfait::StringTools::findAndReplace(name, " ", "_");
            field_names += " " + name;
        }
        for (auto f : cell_fields) {
            auto name = f->name();
            name = Parfait::StringTools::findAndReplace(name, " ", "_");
            field_names += " " + name;
        }

        return field_names;
    }

    void openFile() {
        fileType_e file_type = FULL;
        [[maybe_unused]] int32_t plt = 0;
        [[maybe_unused]] int32_t szplt = 1;
        int32_t fileFormat = szplt;  // only szplt works for HO elements.

        int32_t FLOAT_DATA_TYPE = 1;
        int32_t defaultVarType = FLOAT_DATA_TYPE;

        auto field_names = createSpaceDelimitedFieldNameNoSpaces();

        auto err = tecFileWriterOpen(filename_base.c_str(),
                                     (char*)"Powered By T-infinity",
                                     field_names.c_str(),
                                     fileFormat,
                                     file_type,
                                     defaultVarType,
                                     NULL,  // gridFileHandle
                                     &file_handle);
        TEC_CHECK(err);
    }

    void closeFile() {
        auto err = tecFileWriterClose(&file_handle);
        TEC_CHECK(err);
    }

    int32_t writeZoneHeader(std::string zone_name, std::set<inf::MeshInterface::CellType> types) {
        std::vector<int32_t> cell_shapes;
        std::vector<int32_t> cell_orders;
        std::vector<int32_t> basis_functions;
        std::vector<int64_t> cell_counts;
        for (auto t : types) {
            cell_shapes.push_back(getCellBaseType(t));
            cell_orders.push_back(getCellOrder(t));
            basis_functions.push_back(0);  // I think this means Lagrangian basis...
            cell_counts.push_back(mesh.cellCount(t));
        }

        std::vector<int> field_locations;
        field_locations.push_back(NODE_LOCATION);  // x
        field_locations.push_back(NODE_LOCATION);  // y
        field_locations.push_back(NODE_LOCATION);  // z

        for (int i = 0; i < int(node_fields.size()); i++) {
            field_locations.push_back(NODE_LOCATION);
        }

        for (int i = 0; i < int(cell_fields.size()); i++) {
            field_locations.push_back(CELL_LOCATION);
        }

        int32_t zone_handle;
        int num_sections = types.size();
        auto err = tecZoneCreateFEMixed(file_handle,
                                        zone_name.c_str(),
                                        mesh.nodeCount(),
                                        num_sections,
                                        cell_shapes.data(),
                                        cell_orders.data(),
                                        basis_functions.data(),
                                        cell_counts.data(),
                                        NULL,  // varTypes,
                                        NULL,  // shareVarFromZone,
                                        field_locations.data(),
                                        NULL,                     // passiveVarList,
                                        use_shared_connectivity,  //  shareConnectivityFromZone,
                                        0,                        //  numFaceConnections
                                        some_other_feature,
                                        &zone_handle);
        TEC_CHECK(err);
        return zone_handle;
    }

    void writeNodeFields(const std::set<inf::MeshInterface::CellType>& types, int32_t& zone_handle) {
        writeXYZ(zone_handle);

        int32_t node_count = mesh.nodeCount();
        next_field_index = 3 + 1;  // xyz always come first
        for (auto f : node_fields) {
            auto field_data = inf::FieldTools::extractColumn<float>(*f, 0);
            auto err = tecZoneVarWriteFloatValues(
                file_handle, zone_handle, next_field_index++, 0, node_count, field_data.data());
            TEC_CHECK(err);
        }
    }

    void writeCellTypeSection(inf::MeshInterface::CellType type, int32_t zone_handle) {
        int32_t cell_type_count = mesh.cellCount(type);
        int32_t type_length = mesh.cellTypeLength(type);

        for (auto f : cell_fields) {
            std::vector<float> cell_data = extractCellDataForType(type, *f);
            PARFAIT_ASSERT(int(cell_data.size()) == cell_type_count, "cell data buffer mismatch");

            auto err = tecZoneVarWriteFloatValues(
                file_handle, zone_handle, next_field_index++, 0, cell_type_count, cell_data.data());
            TEC_CHECK(err);
        }

        auto buffer_length = cell_type_count * type_length;
        std::vector<int32_t> cells(buffer_length);
        size_t type_index = 0;
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.cellType(c) == type) {
                auto cell_nodes = mesh.cell(c);
                for (int i = 0; i < type_length; i++) {
                    auto I = type_length * type_index + i;
                    PARFAIT_ASSERT_BOUNDS(cells, I, "cell");
                    cells.at(I) = cell_nodes[i];
                }
                type_index++;
            }
        }

        auto err = tecZoneNodeMapWrite32(file_handle,
                                         zone_handle,
                                         0,  // partition
                                         nodesAreOneBased,
                                         buffer_length,
                                         cells.data());
        TEC_CHECK(err);
    }

    void writeZone(std::string zone_name, const std::set<inf::MeshInterface::CellType>& types) {
        if (types.size() == 0) return;
        int32_t zone_handle = writeZoneHeader(zone_name, types);
        writeNodeFields(types, zone_handle);

        for (auto t : types) {
            printf("writing section %s\n", inf::MeshInterface::cellTypeString(t).c_str());
            writeCellTypeSection(t, zone_handle);
        }
    }

    std::vector<float> extractCellDataForType(inf::MeshInterface::CellType type, const inf::FieldInterface& field) {
        int count = mesh.cellCount(type);
        std::vector<float> data(count);
        size_t index = 0;
        PARFAIT_ASSERT(field.blockSize() == 1, "only scalars");
        for (int c = 0; c < mesh.cellCount(); c++) {
            if (mesh.cellType(c) == type) {
                double d;
                field.value(c, &d);
                data.at(index++) = d;
            }
        }
        return data;
    }

    void writeXYZ(int32_t& zone_handle) {
        int32_t node_count = mesh.nodeCount();
        std::vector<float> x(node_count);
        std::vector<float> y(node_count);
        std::vector<float> z(node_count);
        for (int n = 0; n < node_count; n++) {
            auto p = mesh.node(n);
            x[n] = p[0];
            y[n] = p[1];
            z[n] = p[2];
        }
        int err = 0;
        err += tecZoneVarWriteFloatValues(file_handle, zone_handle, 1, 0, node_count, x.data());
        err += tecZoneVarWriteFloatValues(file_handle, zone_handle, 2, 0, node_count, y.data());
        err += tecZoneVarWriteFloatValues(file_handle, zone_handle, 3, 0, node_count, z.data());
        TEC_CHECK(err);
    }

    int32_t getCellBaseType(const inf::MeshInterface::CellType t) {
        switch (t) {
            case inf::MeshInterface::CellType::BAR_2:
            case inf::MeshInterface::CellType::BAR_3:
                return BAR;
            case inf::MeshInterface::CellType::TRI_3:
            case inf::MeshInterface::CellType::TRI_6:
                return TRI;
            case inf::MeshInterface::CellType::QUAD_4:
            case inf::MeshInterface::CellType::QUAD_9:
                return QUAD;
            case inf::MeshInterface::CellType::TETRA_4:
            case inf::MeshInterface::CellType::TETRA_10:
                return TET;
            case inf::MeshInterface::CellType::PYRA_5:
            case inf::MeshInterface::CellType::PYRA_14:
                return PYRAMID;
            case inf::MeshInterface::CellType::PENTA_6:
            case inf::MeshInterface::CellType::PENTA_18:
                return PENTA;
            case inf::MeshInterface::CellType::HEXA_8:
            case inf::MeshInterface::CellType::HEXA_27:
                return HEX;
            default:
                PARFAIT_THROW("Could not map type " + inf::MeshInterface::cellTypeString(t) + " to a tecio base type");
        }
    }

    int32_t getCellOrder(const inf::MeshInterface::CellType t) {
        switch (t) {
            case inf::MeshInterface::CellType::BAR_2:
            case inf::MeshInterface::CellType::TRI_3:
            case inf::MeshInterface::CellType::QUAD_4:
            case inf::MeshInterface::CellType::TETRA_4:
            case inf::MeshInterface::CellType::PYRA_5:
            case inf::MeshInterface::CellType::PENTA_6:
            case inf::MeshInterface::CellType::HEXA_8:
                return 1;
            case inf::MeshInterface::CellType::BAR_3:
            case inf::MeshInterface::CellType::TRI_6:
            case inf::MeshInterface::CellType::QUAD_9:
            case inf::MeshInterface::CellType::TETRA_10:
            case inf::MeshInterface::CellType::PYRA_14:
            case inf::MeshInterface::CellType::PENTA_18:
            case inf::MeshInterface::CellType::HEXA_27:
                return 2;
            default:
                PARFAIT_THROW("Could not map type " + inf::MeshInterface::cellTypeString(t) + " to a tecio base type");
        }
    }
};

}