#include "TinfMesh.h"
#include <MessagePasser/MessagePasser.h>
#include <parfait/Throw.h>

namespace inf {
void TinfMesh::throwIfDataIncomplete(const TinfMeshData& data) {
    for (auto& x : data.cells) {
        auto& cell_type = x.first;
        auto& cell_list = x.second;
        int cell_length = inf::MeshInterface::cellTypeLength(cell_type);
        int ncells = cell_list.size() / cell_length;
        std::string type_string = inf::MeshInterface::cellTypeString(cell_type);
        if (data.global_cell_id.count(cell_type) == 0)
            throw std::logic_error(__FUNCTION__ +
                                   std::string("missing global cell ids for type: " + type_string));
        if (int(data.global_cell_id.at(cell_type).size()) != ncells)
            throw std::logic_error(
                __FUNCTION__ +
                std::string(" # global cell is != # cells for type: " + type_string));
        if (data.cell_owner.count(cell_type) == 0)
            throw std::logic_error(__FUNCTION__ +
                                   std::string("missing cell owners for type: " + type_string));
        if (int(data.cell_owner.at(cell_type).size()) != ncells)
            throw std::logic_error(
                __FUNCTION__ +
                std::string(" # cell owners is != # cells for type: " + type_string));
        if (data.cell_tags.count(cell_type) == 0)
            throw std::logic_error(__FUNCTION__ +
                                   std::string("missing cell tags for type: " + type_string));
        if (int(data.cell_tags.at(cell_type).size()) != ncells)
            throw std::logic_error(
                __FUNCTION__ + std::string(" # cell tags is != # cells for type: " + type_string));
    }
}

TinfMesh::TinfMesh(std::shared_ptr<MeshInterface> import_mesh) : TinfMesh(*import_mesh) {}

TinfMesh::TinfMesh(const MeshInterface& import_mesh) : my_partition_id(import_mesh.partitionId()) {
    cell_count = import_mesh.cellCount();
    extractCells(import_mesh);
    cell_id_to_type_and_local_id = TinfMeshData::buildCellIdToTypeAndLocalIdMap(mesh);
    extractCellTags(import_mesh);
    extractPoints(import_mesh);
    extractGlobalNodeId(import_mesh);
    extractGlobalCellId(import_mesh);
    extractNodeOwner(import_mesh);
    extractCellOwner(import_mesh);
    extractTagNames(import_mesh);
}

TinfMesh::TinfMesh(TinfMeshData data, int partition_id)
    : mesh(std::move(data)), my_partition_id(partition_id) {
    throwIfDataIncomplete(data);
    cell_count = mesh.countCells();
    cell_id_to_type_and_local_id = TinfMeshData::buildCellIdToTypeAndLocalIdMap(mesh);
}

int TinfMesh::partitionId() const { return my_partition_id; }

int TinfMesh::nodeCount() const { return mesh.points.size(); }

int TinfMesh::cellCount() const { return cell_count; }

int TinfMesh::cellCount(MeshInterface::CellType cell_type) const {
    int cell_length = MeshInterface::cellTypeLength(cell_type);
    if (mesh.cells.count(cell_type) == 0) return 0;
    return mesh.cells.at(cell_type).size() / cell_length;
}

void TinfMesh::nodeCoordinate(int node_id, double* coord) const {
    coord[0] = mesh.points.at(node_id)[0];
    coord[1] = mesh.points.at(node_id)[1];
    coord[2] = mesh.points.at(node_id)[2];
}

std::vector<std::pair<MeshInterface::CellType, int>> TinfMeshData::buildCellIdToTypeAndLocalIdMap(
    const TinfMeshData& mesh_data) {
    int cell_count = mesh_data.countCells();
    std::vector<std::pair<MeshInterface::CellType, int>> cell_id_map(cell_count);
    for (int i = 0; i < cell_count; i++) {
        int id = i;
        for (auto& pair : mesh_data.cells) {
            int cell_length = MeshInterface::cellTypeLength(pair.first);
            if (id < int(pair.second.size() / cell_length)) {
                cell_id_map[i] = std::make_pair(pair.first, id);
                break;
            }
            id -= pair.second.size() / cell_length;
        }
    }
    return cell_id_map;
}

std::pair<MeshInterface::CellType, int> TinfMesh::cellIdToTypeAndLocalId(int cell_id) const {
    return cell_id_to_type_and_local_id.at(cell_id);
}

MeshInterface::CellType TinfMesh::cellType(int cell_id) const {
    auto p = cellIdToTypeAndLocalId(cell_id);
    return p.first;
}

void TinfMesh::cell(int cell_id, int* cell) const {
    int id;
    MeshInterface::CellType type;
    std::tie(type, id) = cellIdToTypeAndLocalId(cell_id);
    int cell_length = cellTypeLength(type);
    for (int i = 0; i < cell_length; i++) {
        try {
            cell[i] = mesh.cells.at(type).at(cell_length * id + i);
        } catch (...) {
            printf("%s %s ERROR accessing cell %d\n", __FILE__, __FUNCTION__, cell_id);
        }
    }
}

long TinfMesh::globalNodeId(int node_id) const { return mesh.global_node_id.at(node_id); }

int TinfMesh::cellTag(int cell_id) const {
    int id;
    MeshInterface::CellType type;
    std::tie(type, id) = cellIdToTypeAndLocalId(cell_id);
    int tag = mesh.cell_tags.at(type).at(id);
    return tag;
}

int TinfMesh::nodeOwner(int node_id) const { return mesh.node_owner.at(node_id); }

void TinfMesh::extractGlobalCellId(const MeshInterface& import_mesh) {
    int ncells = import_mesh.cellCount();
    for (int n = 0; n < ncells; n++) {
        auto cell_type = import_mesh.cellType(n);
        mesh.global_cell_id[cell_type].push_back(import_mesh.globalCellId(n));
    }
}

void TinfMesh::extractCellOwner(const MeshInterface& import_mesh) {
    int ncells = import_mesh.cellCount();
    for (int n = 0; n < ncells; n++) {
        auto cell_type = import_mesh.cellType(n);
        int owner = import_mesh.cellOwner(n);
        mesh.cell_owner[cell_type].push_back(owner);
    }
}

void TinfMesh::extractNodeOwner(const MeshInterface& import_mesh) {
    mesh.node_owner.resize(import_mesh.nodeCount());
    for (int n = 0; n < int(mesh.node_owner.size()); n++) {
        mesh.node_owner[n] = import_mesh.nodeOwner(n);
    }
}

void TinfMesh::extractGlobalNodeId(const MeshInterface& import_mesh) {
    mesh.global_node_id.resize(import_mesh.nodeCount());
    for (int n = 0; n < int(mesh.global_node_id.size()); n++) {
        mesh.global_node_id[n] = import_mesh.globalNodeId(n);
    }
}

void TinfMesh::extractCells(const MeshInterface& import_mesh) {
    for (int cell_id = 0; cell_id < cell_count; cell_id++) {
        auto type = import_mesh.cellType(cell_id);
        int length = MeshInterface::cellTypeLength(type);
        size_t next_index = mesh.cells[type].size();
        mesh.cells[type].resize(mesh.cells.at(type).size() + length);
        int* cell = &mesh.cells.at(type).at(next_index);
        import_mesh.cell(cell_id, cell);
    }
}

void TinfMesh::extractCellTags(const MeshInterface& import_mesh) {
    for (int cell_id = 0; cell_id < cell_count; cell_id++) {
        auto type = import_mesh.cellType(cell_id);
        int tag = import_mesh.cellTag(cell_id);
        mesh.cell_tags[type].push_back(tag);
    }
}

void TinfMesh::extractPoints(const MeshInterface& mesh_in) {
    mesh.points.resize(mesh_in.nodeCount());
    for (int n = 0; n < int(mesh.points.size()); n++) {
        mesh.points[n] = mesh_in.node(n);
    }
}

void TinfMesh::setNodeCoordinate(int node_id, double* coord) {
    mesh.points[node_id][0] = coord[0];
    mesh.points[node_id][1] = coord[1];
    mesh.points[node_id][2] = coord[2];
}
void TinfMesh::setCell(int cell_id, const std::vector<int>& cell) {
    int id;
    MeshInterface::CellType type;
    std::tie(type, id) = cellIdToTypeAndLocalId(cell_id);
    int length = cellTypeLength(type);
    if (cell.size() != size_t(length))
        throw std::logic_error("TinfMesh trying to set a cell of mismatched length");
    int start = length * id;
    auto& vec = mesh.cells[type];
    for (int i = 0; i < length; i++) {
        size_t index = start + i;
        vec[index] = cell[i];
    }
}

long TinfMesh::globalCellId(int cell_id) const {
    auto type_and_id = cellIdToTypeAndLocalId(cell_id);
    if (mesh.global_cell_id.count(type_and_id.first) == 0) {
        PARFAIT_THROW("Map At error " + std::to_string(type_and_id.first));
    }
    return mesh.global_cell_id.at(type_and_id.first).at(type_and_id.second);
}

int TinfMesh::cellOwner(int cell_id) const {
    auto type_and_id = cellIdToTypeAndLocalId(cell_id);
    if (mesh.cell_owner.count(type_and_id.first) == 0) {
        PARFAIT_THROW("Map At error " + std::to_string(type_and_id.first));
    }
    return mesh.cell_owner.at(type_and_id.first).at(type_and_id.second);
}
std::set<long> buildResidentNodeSet(const inf::TinfMeshData& mesh) {
    std::set<long> resident_nodes(mesh.global_node_id.begin(), mesh.global_node_id.end());
    return resident_nodes;
}
std::map<long, int> buildGlobalToLocalNode(const inf::TinfMeshData& mesh) {
    std::map<long, int> g2l;
    for (int local = 0; local < int(mesh.global_node_id.size()); local++) {
        long global = mesh.global_node_id[local];
        g2l[global] = local;
    }
    return g2l;
}
TinfMeshCell TinfMeshData::getCell(inf::MeshInterface::CellType type, int index) const {
    TinfMeshCell cell;
    int length = inf::MeshInterface::cellTypeLength(type);
    cell.type = type;
    cell.global_id = global_cell_id.at(type).at(index);
    cell.tag = cell_tags.at(type).at(index);
    cell.owner = cell_owner.at(type).at(index);
    for (int i = 0; i < length; i++) {
        int node_lid = cells.at(type).at(index * length + i);
        long node_gid = global_node_id.at(node_lid);
        cell.nodes.push_back(node_gid);
        cell.points.push_back(points.at(node_lid));
        cell.node_owner.push_back(node_owner.at(node_lid));
    }
    return cell;
}
void TinfMeshData::pack(MessagePasser::Message& m) const {
    m.pack(cells);
    m.pack(cell_tags);
    m.pack(points);
    m.pack(global_node_id);
    m.pack(node_owner);
    m.pack(global_cell_id);
    m.pack(cell_owner);
}
void TinfMeshData::unpack(MessagePasser::Message& m) {
    m.unpack(cells);
    m.unpack(cell_tags);
    m.unpack(points);
    m.unpack(global_node_id);
    m.unpack(node_owner);
    m.unpack(global_cell_id);
    m.unpack(cell_owner);
}
int TinfMeshData::countCells() const {
    int cell_count = 0;
    for (auto& pair : cells) {
        MeshInterface::CellType cell_type = pair.first;
        int cell_length = inf::MeshInterface::cellTypeLength(cell_type);
        cell_count += pair.second.size() / cell_length;
    }
    return cell_count;
}
TinfMeshCell TinfMesh::getCell(int cell_id) const {
    auto pair = cellIdToTypeAndLocalId(cell_id);
    return mesh.getCell(pair.first, pair.second);
}
std::string TinfMesh::to_string() const {
    std::string m;

    m += "Number of nodes: " + std::to_string(nodeCount()) + "\n";
    m += "Number of cells: " + std::to_string(cellCount()) + "\n";

    for (auto& pair : mesh.cells) {
        auto type = pair.first;
        auto& nodes = pair.second;
        int length = MeshInterface::cellTypeLength(type);
        m += MeshInterface::cellTypeString(type) + " cells:\n";

        int num_cells = int(nodes.size()) / length;
        for (int c = 0; c < num_cells; c++) {
            m += "gid: " + std::to_string(mesh.global_cell_id.at(type)[c]);
            for (int i = 0; i < length; i++) {
                m += " " + std::to_string(nodes[length * c + i]);
            }
            m += "\n";
        }
    }

    return m;
}

void TinfMesh::rebuild() {
    throwIfDataIncomplete(mesh);
    cell_count = mesh.countCells();
    cell_id_to_type_and_local_id = TinfMeshData::buildCellIdToTypeAndLocalIdMap(mesh);
}
std::vector<int> TinfMesh::cell(MeshInterface::CellType type, int type_index) {
    auto length = MeshInterface::cellTypeLength(type);
    std::vector<int> nodes(length);
    auto& type_cells = mesh.cells.at(type);
    for (int i = 0; i < length; i++) {
        nodes[i] = type_cells[length * type_index + i];
    }
    return nodes;
}
std::string TinfMesh::tagName(int t) const {
    if (mesh.tag_names.count(t) == 0)
        return "Tag_" + std::to_string(t);
    else
        return mesh.tag_names.at(t);
}
void TinfMesh::setTagName(int t, std::string name) { mesh.tag_names[t] = name; }
void TinfMesh::extractTagNames(const MeshInterface& import_mesh) {
    for (int cell_id = 0; cell_id < cellCount(); ++cell_id) {
        int tag = cellTag(cell_id);
        if (mesh.tag_names.count(tag) == 0) mesh.tag_names[tag] = import_mesh.tagName(tag);
    }
}
}
