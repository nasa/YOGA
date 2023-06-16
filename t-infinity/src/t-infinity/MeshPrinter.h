#pragma once
#include <MessagePasser/MessagePasser.h>
#include "MeshInterface.h"
#include <parfait/ToString.h>

namespace inf {
namespace MeshPrinter {
    inline std::string toBuildableSerialMeshString(const inf::TinfMesh& m) {
        auto ts = [](auto t) { return std::to_string(t); };
        std::string output;
        output = "auto builder = std::make_unique<inf::experimental::MeshBuilder>(mp);\n";
        for (int n = 0; n < m.nodeCount(); n++) {
            auto p = m.node(n);
            output += "builder->addNode({" + Parfait::to_string(p, ",") + "});\n";
        }
        for (auto& pair : m.mesh.cells) {
            auto type = pair.first;
            auto length = inf::MeshInterface::cellTypeLength(type);
            auto type_s = "inf::MeshInterface::" + inf::MeshInterface::cellTypeString(type);
            int count = m.mesh.cells.at(type).size() / length;
            for (int c = 0; c < count; c++) {
                std::vector<int> cell_nodes(length);
                for (int i = 0; i < length; i++) {
                    cell_nodes[i] = m.mesh.cells.at(type)[c * length + i];
                }
                int tag = m.mesh.cell_tags.at(type)[c];
                output += "builder->addCell(" + type_s + ",{" +
                          Parfait::to_string(cell_nodes, ",") + "}, " + ts(tag) + ");\n";
            }
        }
        output += "builder->sync();\n";
        output += "auto mesh = builder->mesh;\n";
        output += "builder.reset(nullptr);\n";
        return output;
    }
    inline std::string toTestFixtureString(MessagePasser mp, const inf::TinfMesh& m) {
        auto ts = [](auto t) { return std::to_string(t); };
        std::string output;
        output += "inf::TinfMesh mesh(inf::TinfMeshData(), mp.Rank());\n";
        output += "mesh.mesh.points.resize(" + ts(m.nodeCount()) + ");\n";
        output += "mesh.mesh.global_node_id.resize(" + ts(m.nodeCount()) + ");\n";
        output += "mesh.mesh.node_owner.resize(" + ts(m.nodeCount()) + ");\n";
        for (int n = 0; n < m.nodeCount(); n++) {
            auto p = m.node(n);
            output += "mesh.mesh.points[" + ts(n) + "] = {" + ts(p[0]) + ", " + ts(p[1]) + ", " +
                      ts(p[2]) + "};\n";
        }
        for (int n = 0; n < m.nodeCount(); n++) {
            output += "mesh.mesh.global_node_id[" + ts(n) + "] = " + ts(m.globalNodeId(n)) + ";\n";
        }
        for (int n = 0; n < m.nodeCount(); n++) {
            output += "mesh.mesh.node_owner[" + ts(n) + "] = " + ts(m.nodeOwner(n)) + ";\n";
        }
        output += "\n";
        for (auto& pair : m.mesh.cells) {
            auto type = pair.first;
            auto length = inf::MeshInterface::cellTypeLength(type);
            auto type_s = "inf::MeshInterface::" + inf::MeshInterface::cellTypeString(type);
            int count = m.mesh.cells.at(type).size() / length;
            output += "mesh.mesh.cells[" + type_s + "].resize(" + ts(count * length) + ");\n";
            output += "mesh.mesh.global_cell_id[" + type_s + "].resize(" + ts(count) + ");\n";
            output += "mesh.mesh.cell_tag[" + type_s + "].resize(" + ts(count) + ");\n";
            output += "mesh.mesh.cell_owner[" + type_s + "].resize(" + ts(count) + ");\n";
            for (int c = 0; c < count; c++) {
                for (int i = 0; i < length; i++) {
                    output += "mesh.mesh.cells[" + type_s + "][" + ts(c) +
                              "] = " + ts(m.mesh.cells.at(type)[c * length + i]) + ";\n";
                }
            }

            for (int c = 0; c < count; c++) {
                output += "mesh.mesh.global_cell_id[" + type_s + "][" + ts(c) +
                          "] = " + ts(m.mesh.global_cell_id.at(type)[c]) + ";\n";
            }

            for (int c = 0; c < count; c++) {
                output += "mesh.mesh.cell_owner[" + type_s + "][" + ts(c) +
                          "] = " + ts(m.mesh.cell_owner.at(type)[c]) + ";\n";
            }

            for (int c = 0; c < count; c++) {
                output += "mesh.mesh.cell_tag[" + type_s + "][" + ts(c) +
                          "] = " + ts(m.mesh.cell_tags.at(type)[c]) + ";\n";
            }
            output += "\n";
        }
        return output;
    }
    inline void printSurfaceCellNodes(MessagePasser mp, const inf::MeshInterface& mesh) {
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            std::vector<int> cell_nodes;
            if (mp.Rank() == r) {
                printf("Rank %d has surface nodes:\n", mp.Rank());
                for (int c = 0; c < mesh.cellCount(); c++) {
                    if (mesh.is2DCell(c)) {
                        mesh.cell(c, cell_nodes);
                        int tag = mesh.cellTag(c);
                        long gid = mesh.globalCellId(c);
                        printf("Local %d, Global %ld, Tag %d Type %s\n",
                               c,
                               gid,
                               tag,
                               mesh.cellTypeString(mesh.cellType(c)).c_str());
                        printf("  Local Nodes: %s\n", Parfait::to_string(cell_nodes).c_str());
                        printf("  Global Nodes: ");
                        for (int n : cell_nodes) {
                            printf("%ld ", mesh.globalNodeId(n));
                        }
                        printf("\n");
                    }
                    printf("\n");
                }
            }
        }
    }
}
}
