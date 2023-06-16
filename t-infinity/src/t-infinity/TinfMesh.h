#pragma once
#include <map>
#include <memory>
#include <set>
#include <array>
#include <string>
#include <utility>
#include <vector>
#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include <t-infinity/MeshInterface.h>
namespace inf {
class TinfMeshCell {
  public:
    TinfMeshCell() = default;
    inline TinfMeshCell(const inf::MeshInterface::CellType& type,
                        long global_id,
                        int tag,
                        int owner,
                        std::vector<long> nodes,
                        std::vector<Parfait::Point<double>> points,
                        std::vector<int> node_owner)
        : type(type),
          global_id(global_id),
          tag(tag),
          owner(owner),
          nodes(std::move(nodes)),
          points(std::move(points)),
          node_owner(std::move(node_owner)) {}
    inline TinfMeshCell(const inf::MeshInterface& mesh, int cell_id) {
        type = mesh.cellType(cell_id);
        global_id = mesh.globalCellId(cell_id);
        tag = mesh.cellTag(cell_id);
        owner = mesh.cellOwner(cell_id);
        std::vector<int> cell_nodes_local(mesh.cellTypeLength(type));
        mesh.cell(cell_id, cell_nodes_local.data());
        for (auto local : cell_nodes_local) {
            nodes.push_back(mesh.globalNodeId(local));
            Parfait::Point<double> p(mesh.node(local).data());
            points.push_back(p);
            node_owner.push_back(mesh.nodeOwner(local));
        }
    }
    inf::MeshInterface::CellType type;
    long global_id;
    int tag;
    int owner;
    std::vector<long> nodes;
    std::vector<Parfait::Point<double>> points;
    std::vector<int> node_owner;
};
struct TinfMeshData {
    std::map<MeshInterface::CellType, std::vector<int>> cells;
    std::map<MeshInterface::CellType, std::vector<int>> cell_tags;
    std::vector<Parfait::Point<double>> points;
    std::vector<long> global_node_id;
    std::vector<int> node_owner;
    std::map<MeshInterface::CellType, std::vector<long>> global_cell_id;
    std::map<MeshInterface::CellType, std::vector<int>> cell_owner;
    std::map<int, std::string> tag_names;

    TinfMeshCell getCell(inf::MeshInterface::CellType type, int index) const;
    void pack(MessagePasser::Message& m) const;
    void unpack(MessagePasser::Message& m);
    int countCells() const;
    static std::vector<std::pair<MeshInterface::CellType, int>> buildCellIdToTypeAndLocalIdMap(
        const TinfMeshData& mesh_data);
};

class TinfMesh : public MeshInterface {
  public:
    using MeshInterface::cell;
    TinfMesh() = default;

    explicit TinfMesh(std::shared_ptr<MeshInterface> import_mesh);
    explicit TinfMesh(const MeshInterface& import_mesh);
    explicit TinfMesh(TinfMeshData data, int partition_id);

    virtual int nodeCount() const override;
    virtual int cellCount() const override;
    virtual int partitionId() const override;
    virtual int cellCount(MeshInterface::CellType cell_type) const override;
    virtual void nodeCoordinate(int node_id, double* coord) const override;
    virtual MeshInterface::CellType cellType(int cell_id) const override;
    virtual void cell(int cell_id, int* cell) const override;
    virtual long globalNodeId(int node_id) const override;
    virtual long globalCellId(int cell_id) const override;
    virtual int cellTag(int cell_id) const override;
    virtual int nodeOwner(int node_id) const override;
    virtual int cellOwner(int cell_id) const override;
    virtual std::string tagName(int t) const override;

    void setNodeCoordinate(int node_id, double* coord);
    void setCell(int cell_id, const std::vector<int>& cell);
    void setTagName(int t, std::string name);
    std::string to_string() const;
    std::vector<int> cell(MeshInterface::CellType type, int type_index);

  public:
    TinfMeshData mesh;
    int my_partition_id;
    int cell_count;
    std::vector<std::pair<CellType, int>> cell_id_to_type_and_local_id;

    void rebuild();
    TinfMeshCell getCell(int cell_id) const;
    std::pair<MeshInterface::CellType, int> cellIdToTypeAndLocalId(int cell_id) const;

    void extractCells(const MeshInterface& import_mesh);
    void extractCellTags(const MeshInterface& import_mesh);
    void extractPoints(const MeshInterface& mesh);
    void extractGlobalNodeId(const MeshInterface& import_mesh);
    void extractNodeOwner(const MeshInterface& import_mesh);
    void extractCellOwner(const MeshInterface& import_mesh);
    void extractGlobalCellId(const MeshInterface& import_mesh);
    void extractTagNames(const MeshInterface& import_mesh);
    void throwIfDataIncomplete(const TinfMeshData& data);
};
std::set<long> buildResidentNodeSet(const inf::TinfMeshData& mesh);
std::map<long, int> buildGlobalToLocalNode(const inf::TinfMeshData& mesh);
}
