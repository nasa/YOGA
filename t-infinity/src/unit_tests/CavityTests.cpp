#include <RingAssertions.h>
#include "MockMeshes.h"
#include <parfait/Throw.h>
#include <t-infinity/Shortcuts.h>

namespace inf  {
struct MutableMesh {
    TinfMeshData mesh;
    MutableMesh(TinfMeshData mesh) : mesh(mesh){
        next_cell_gid = findLargestGlobalCellId() + 1;
        next_node_gid = findLargestGlobalNodeId() + 1;
    }
    long next_cell_gid;
    long next_node_gid;
    int my_rank = 0;

    std::set<long> deleted_node_gids;
    std::set<long> deleted_cell_gids;
    std::set<long> new_node_gids;
    std::set<long> new_cell_gids;

    std::map<MeshInterface::CellType, std::set<int>> deleted_cell_type_indices;
    std::set<int> deleted_node_local_ids;

    void deleteNode(int node_id){
        deleted_node_local_ids.insert(node_id);
        auto gid = mesh.global_node_id.at(node_id);
        deleted_node_gids.insert(gid);
    }

    void deleteCell(inf::MeshInterface::CellType type, int cell_index) {
        deleted_cell_type_indices[type].insert(cell_index);
        auto gid = mesh.global_cell_id.at(type).at(cell_index);
        deleted_cell_gids.insert(gid);
    }

    int addNode(Parfait::Point<double> p){
        long gid = claimNextNodeGid();
        if(deleted_node_local_ids.empty()){
            mesh.points.push_back(p);
            mesh.global_node_id.push_back(gid);
            mesh.node_owner.push_back(my_rank);
            return int(mesh.points.size())-1;
        } else {
            int index_to_overwrite = *deleted_node_local_ids.begin();
            deleted_node_local_ids.erase(index_to_overwrite);
            mesh.points[index_to_overwrite] = p;
            mesh.global_node_id[index_to_overwrite] = gid;
            mesh.node_owner[index_to_overwrite] = my_rank;
            return index_to_overwrite;
        }
    }

    void addCell(inf::MeshInterface::CellType type, const std::vector<int>& cell_nodes, int tag = 0) {
        int length = inf::MeshInterface::cellTypeLength(type);
        if(int(cell_nodes.size()) != length){
            PARFAIT_THROW("Can not add " + std::to_string(cell_nodes.size()) + " nodes as a cell of type " + inf::MeshInterface::cellTypeString(type));
        }
        long gid = claimNextCellGid();
        if(deleted_cell_type_indices.count(type) == 0){
            deleted_cell_type_indices[type] = {};
        }
        if (deleted_cell_type_indices.at(type).empty()) {
            int new_cell_index = mesh.cells[type].size() / length;
            mesh.cells[type].resize(mesh.cells[type].size() + length);
            mesh.cell_owner[type].push_back(my_rank);
            mesh.cell_tags[type].push_back(tag);
            mesh.global_cell_id[type].push_back(gid);
            for (int i = 0; i < length; i++) mesh.cells[type][length * new_cell_index + i] = cell_nodes[i];

        } else {
            int index_to_overwrite = *deleted_cell_type_indices.at(type).begin();
            deleted_cell_type_indices.at(type).erase(index_to_overwrite);
            mesh.cell_owner[type][index_to_overwrite] = my_rank;
            mesh.cell_tags[type][index_to_overwrite] = tag;
            mesh.global_cell_id[type][index_to_overwrite] = gid;
            for (int i = 0; i < length; i++) mesh.cells[type][length * index_to_overwrite + i] = cell_nodes[i];
        }
    }

    long claimNextCellGid() {
        if(deleted_cell_gids.empty()){
            new_cell_gids.insert(next_cell_gid);
            next_cell_gid++;
            return next_cell_gid-1;
        }
        else {
            long next = *deleted_cell_gids.begin();
            deleted_cell_gids.erase(next);
            return next;
        }
    }
    long claimNextNodeGid() {
        if(deleted_node_gids.empty()){
            new_node_gids.insert(next_node_gid);
            next_node_gid++;
            return next_node_gid-1;
        }
        else {
            long next = *deleted_node_gids.begin();
            deleted_node_gids.erase(next);
            return next;
        }
    }

    long findLargestGlobalCellId() const {
        long biggest = -1;
        for(auto& pair : mesh.global_cell_id){
            auto& gids = pair.second;
            for(auto g : gids){
                biggest = std::max(biggest, g);
            }
        }
        return biggest;
    }
    long findLargestGlobalNodeId() const {
        long biggest = -1;
        for(auto g : mesh.global_node_id){
            biggest = std::max(biggest, g);
        }
        return biggest;
    }
    void finalizeLazyDeletions() {
        for(auto pair : deleted_cell_type_indices) {
            auto type = pair.first;
            auto to_be_deleted = pair.second;
            int length = MeshInterface::cellTypeLength(type);
            for (int id : to_be_deleted) {
                auto& cells = mesh.cells[type];
                auto& owners = mesh.cell_owner[type];
                auto& tags = mesh.cell_tags[type];
                auto& gids = mesh.global_cell_id[type];
                int last = cells.size() / length;
                for (int i = 0; i < length; i++) cells[id * length + i] = cells[last * length + i];
                cells.resize(cells.size() - length);
                owners[id] = owners[last];
                owners.pop_back();
                tags[id] = tags.back();
                tags.pop_back();
                gids[id] = gids.back();
            }
            if(mesh.cells[type].empty()) {
                mesh.cells.erase(type);
                mesh.cell_owner.erase(type);
                mesh.cell_tags.erase(type);
                mesh.global_cell_id.erase(type);
            }
        }
    }
};
}

TEST_CASE("Explore cavity operator") {
    using CellType = inf::MeshInterface::CellType;
    auto mesh = inf::mock::hangingEdge();

    inf::MutableMesh mutable_mesh(mesh);
    REQUIRE(mutable_mesh.findLargestGlobalCellId() == 13);
    REQUIRE(mutable_mesh.mesh.cells.at(CellType::HEXA_8).size() / 8 == 1);

    mutable_mesh.deleteCell(CellType::TRI_3, 0);
    REQUIRE(mutable_mesh.deleted_cell_type_indices.at(CellType::TRI_3).size() == 1);
    REQUIRE(mutable_mesh.deleted_cell_gids.count(12) == 1);

    mutable_mesh.deleteCell(CellType::QUAD_4, 0);
    REQUIRE(mutable_mesh.deleted_cell_type_indices.at(inf::MeshInterface::QUAD_4).size() == 1);
    REQUIRE(mutable_mesh.deleted_cell_gids.count(3) == 1);

    mutable_mesh.addCell(CellType::TRI_3, {0, 3, 2});
    REQUIRE(mutable_mesh.mesh.cells[CellType::TRI_3].size() / 3 == 2);

    mutable_mesh.addCell(CellType::TRI_3, {2, 1, 0});
    REQUIRE(mutable_mesh.mesh.cells[CellType::TRI_3].size() / 3 == 3);
    REQUIRE(mutable_mesh.mesh.cell_owner[CellType::TRI_3].size() == 3);
    REQUIRE(mutable_mesh.mesh.cell_tags[CellType::TRI_3].size() == 3);
    REQUIRE(mutable_mesh.mesh.global_cell_id[CellType::TRI_3][2] == 12);

    REQUIRE_THROWS(mutable_mesh.addCell(CellType::TRI_3, {0,1,2,3,4}));
}
TEST_CASE("Remesh hanging cell") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    using CellType = inf::MeshInterface::CellType;
    auto mesh = inf::mock::hangingEdge();

    auto ptr = std::make_shared<inf::TinfMesh>(mesh,0);
    inf::shortcut::visualize("fixed-hanging-edge-starting", mp, ptr);

    inf::MutableMesh mutable_mesh(mesh);

    mutable_mesh.deleteCell(CellType::HEXA_8, 0);
    int new_node = mutable_mesh.addNode({.5, .5, .5});
    mutable_mesh.addCell(CellType::PYRA_5, {0,1,2,3,new_node});
    mutable_mesh.addCell(CellType::PYRA_5, {0,4,5,1,new_node});
    mutable_mesh.addCell(CellType::PYRA_5, {1,5,6,2,new_node});
    mutable_mesh.addCell(CellType::PYRA_5, {2,6,7,3,new_node});
    mutable_mesh.addCell(CellType::PYRA_5, {3,7,4,0,new_node});
    mutable_mesh.addCell(CellType::TETRA_4, {5,7,6,new_node});
    mutable_mesh.addCell(CellType::TETRA_4, {7,5,4,new_node});


    REQUIRE(mutable_mesh.mesh.cells[CellType::HEXA_8].size() / 8 == 1);

    mutable_mesh.finalizeLazyDeletions();
    REQUIRE(mutable_mesh.mesh.cells.count(CellType::HEXA_8) == 0);
    REQUIRE(mutable_mesh.mesh.cell_owner.count(CellType::HEXA_8) == 0);
    REQUIRE(mutable_mesh.mesh.cell_tags.count(CellType::HEXA_8) == 0);
    REQUIRE(mutable_mesh.mesh.global_cell_id.count(CellType::HEXA_8) == 0);


    auto new_mesh = std::make_shared<inf::TinfMesh>(mutable_mesh.mesh, 0);
    inf::shortcut::visualize("fixed-hanging-edge", mp, new_mesh);
}
