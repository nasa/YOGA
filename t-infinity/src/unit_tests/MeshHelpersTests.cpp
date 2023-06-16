#include <RingAssertions.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/AddHalo.h>
#include <t-infinity/Extract.h>
#include "parfait/SyncField.h"
#include "t-infinity/GlobalToLocal.h"
#include "t-infinity/MeshConnectivity.h"

using namespace inf;

TEST_CASE("Can build sync pattern from mesh") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = CartMesh::create(mp, 10, 10, 10);
    SECTION("Sync nodes") {
        std::vector<long> global_node_ids(mesh->nodeCount(), -1);
        for (int node = 0; node < mesh->nodeCount(); ++node) {
            if (mesh->ownedNode(node)) global_node_ids[node] = mesh->globalNodeId(node);
        }
        auto node_sync_pattern = buildNodeSyncPattern(mp, *mesh);
        Parfait::syncVector(mp, global_node_ids, GlobalToLocal::buildNode(*mesh), node_sync_pattern);
        for (int node = 0; node < mesh->nodeCount(); ++node) {
            REQUIRE(mesh->globalNodeId(node) == global_node_ids[node]);
        }
    }
    SECTION("Sync cells") {
        std::vector<long> global_cell_ids(mesh->cellCount(), -1);
        for (int cell = 0; cell < mesh->cellCount(); ++cell) {
            if (mesh->ownedCell(cell)) global_cell_ids[cell] = mesh->globalCellId(cell);
        }
        auto cell_sync_pattern = buildCellSyncPattern(mp, *mesh);
        Parfait::syncVector(mp, global_cell_ids, GlobalToLocal::buildCell(*mesh), cell_sync_pattern);
        for (int cell = 0; cell < mesh->cellCount(); ++cell) {
            REQUIRE(mesh->globalCellId(cell) == global_cell_ids[cell]);
        }
    }
}
TEST_CASE("Can build distance-based sync pattern from mesh") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = addHaloNodes(mp, *CartMesh::create(mp, 20, 20, 20));

    auto n2n = NodeToNode::build(*mesh);
    SECTION("Can get distance-1/-2 ghost nodes") {
        std::set<int> expected_distance_1_ghost_nodes;
        std::set<int> expected_distance_2_ghost_nodes;
        auto addNeighbors = [&](int node_id, auto& node_ids) {
            for (int n : n2n[node_id]) {
                if (not mesh->ownedNode(n)) node_ids.insert(n);
            }
        };
        for (int node_id = 0; node_id < mesh->nodeCount(); ++node_id) {
            if (mesh->ownedNode(node_id)) {
                addNeighbors(node_id, expected_distance_1_ghost_nodes);
                addNeighbors(node_id, expected_distance_2_ghost_nodes);
                for (int n : n2n[node_id]) {
                    addNeighbors(n, expected_distance_2_ghost_nodes);
                }
            }
        }
        REQUIRE(expected_distance_1_ghost_nodes == extractResidentButNotOwnedNodesByDistance(*mesh, 1, n2n));
        REQUIRE(expected_distance_2_ghost_nodes == extractResidentButNotOwnedNodesByDistance(*mesh, 2, n2n));
    }
    SECTION("Can sync distance-1 only") {
        std::vector<long> global_node_ids(mesh->nodeCount(), -1);
        for (int node = 0; node < mesh->nodeCount(); ++node) {
            if (mesh->ownedNode(node)) global_node_ids[node] = mesh->globalNodeId(node);
        }
        auto distance_1_ghosts = extractResidentButNotOwnedNodesByDistance(*mesh, 1, n2n);
        auto node_sync_pattern = buildNodeSyncPattern(mp, *mesh, distance_1_ghosts);
        Parfait::syncVector(mp, global_node_ids, GlobalToLocal::buildNode(*mesh), node_sync_pattern);
        for (int node = 0; node < mesh->nodeCount(); ++node) {
            if (mesh->ownedNode(node) or distance_1_ghosts.count(node) == 1) {
                REQUIRE(mesh->globalNodeId(node) == global_node_ids[node]);
            } else {
                REQUIRE(-1 == global_node_ids[node]);
            }
        }
    }
}
