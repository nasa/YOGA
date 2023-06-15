#include "VisualizePartition.h"

#include <memory>

#include "PluginLoader.h"
#include "VectorFieldAdapter.h"
#include "PluginLocator.h"
#include "Shortcuts.h"

namespace inf {

class SerialMesh : public MeshInterface {
  public:
    SerialMesh(const MeshInterface& mesh) : mesh(mesh) {}
    int nodeCount() const override { return mesh.nodeCount(); }
    int cellCount() const override { return mesh.cellCount(); }
    int cellCount(CellType cell_type) const override { return mesh.cellCount(cell_type); }
    int partitionId() const override { return 0; }
    void nodeCoordinate(int node_id, double* coord) const override {
        return mesh.nodeCoordinate(node_id, coord);
    }

    CellType cellType(int cell_id) const override { return mesh.cellType(cell_id); }
    void cell(int cell_id, int* cell_out) const override { return mesh.cell(cell_id, cell_out); }

    long globalNodeId(int node_id) const override { return node_id; }
    int cellTag(int cell_id) const override { return mesh.cellTag(cell_id); }
    int nodeOwner(int node_id) const override { return 0; }
    long globalCellId(int cell_id) const override { return cell_id; }
    int cellOwner(int cell_id) const override { return 0; }

  private:
    const MeshInterface& mesh;
};

void visualizePartition(std::string filename_base,
                        const MeshInterface& mesh,
                        const std::vector<std::shared_ptr<inf::FieldInterface>>& fields,
                        std::string viz_plugin_name,
                        std::string plugin_directory) {
    auto serial_mesh = std::make_shared<SerialMesh>(mesh);
    std::vector<double> is_owned_cell(mesh.cellCount());
    for (int c = 0; c < mesh.cellCount(); c++) {
        is_owned_cell[c] = mesh.ownedCell(c);
    }
    std::vector<double> is_owned_node(mesh.nodeCount(), 0);
    for (int n = 0; n < mesh.nodeCount(); n++) {
        is_owned_node[n] = mesh.ownedNode(n);
    }
    auto f_cell = std::make_shared<VectorFieldAdapter>(
        "is_owned_cell", FieldAttributes::Cell(), 1, is_owned_cell);
    auto f_node = std::make_shared<VectorFieldAdapter>(
        "is_owned_node", FieldAttributes::Node(), 1, is_owned_node);

    std::string filename = filename_base + std::to_string(mesh.partitionId());
    auto viz = shortcut::loadViz(filename, serial_mesh, MessagePasser(MPI_COMM_SELF));
    viz->addField(f_cell);
    viz->addField(f_node);
    for (auto& f : fields) {
        viz->addField(f);
    }
    viz->visualize();
}

}
