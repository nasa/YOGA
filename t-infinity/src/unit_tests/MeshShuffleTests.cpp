#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/MeshShuffle.h>
#include <t-infinity/Extract.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/MeshSanityChecker.h>
#include <parfait/ToString.h>
#include <parfait/VectorTools.h>
#include <t-infinity/Cell.h>

TEST_CASE("MeshShuffle Will refuse to shuffle to a nonsensical distribution") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(2, 2, 2);

    int new_owner = -1;
    std::vector<int> new_node_owners(mesh->nodeCount(), new_owner);

    REQUIRE_THROWS(inf::MeshShuffle::shuffleNodes(mp, *mesh, new_node_owners));
}

TEST_CASE("Shuffle a mesh by assigning a cell") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() == 1) return;

    std::shared_ptr<inf::TinfMesh> mesh = nullptr;
    if(mp.Rank() == 0){
        mesh = inf::CartMesh::create(3, 3, 1);
    } else {
        mesh = std::make_shared<inf::TinfMesh>(inf::TinfMeshData(), mp.Rank());
    }
    mesh->setTagName(1, "pikachu");

    std::vector<int> new_cell_owners(mesh->cellCount(), 0);
    if (mp.Rank() == 0) {
        new_cell_owners[0] = 1;
    }

    auto shuffled_mesh = inf::MeshShuffle::shuffleCells(mp, *mesh, new_cell_owners);
    REQUIRE(shuffled_mesh->nodeCount() > 1);
    int owned_cell_count = 0;
    for (int c = 0; c < shuffled_mesh->cellCount(); c++) {
        if (shuffled_mesh->partitionId() == shuffled_mesh->cellOwner(c)) {
            owned_cell_count++;
        }
    }

    if (mp.Rank() == 0) {
        REQUIRE(owned_cell_count > 4);
    } else if (mp.Rank() == 1) {
        REQUIRE(owned_cell_count == 1);
    } else {
        REQUIRE(owned_cell_count == 0);
    }
    REQUIRE(shuffled_mesh->tagName(1) == "pikachu");
}

TEST_CASE("Shuffle a mesh by putting one node on the remote processor") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() == 1) return;

    std::shared_ptr<inf::MeshInterface> mesh = nullptr;
    if(mp.Rank() == 0){
        mesh = inf::CartMesh::create( 5, 5, 5);
    } else {
        mesh = std::make_shared<inf::TinfMesh>(inf::TinfMeshData(), mp.Rank());
    }


    std::vector<int> new_node_owners(mesh->nodeCount(), 0);
    if (mp.Rank() == 0) {
        new_node_owners[0] = 1;
    }
    auto shuffled_mesh = inf::MeshShuffle::shuffleNodes(mp, *mesh, new_node_owners);
    REQUIRE(shuffled_mesh != nullptr);

    int owned_node_count = 0;
    for (int n = 0; n < shuffled_mesh->nodeCount(); n++) {
        if (shuffled_mesh->partitionId() == shuffled_mesh->nodeOwner(n)) {
            owned_node_count++;
        }
    }

    if (mp.Rank() == 0) {
        REQUIRE(owned_node_count == 215);
    } else if (mp.Rank() == 1) {
        REQUIRE(owned_node_count == 1);
    } else {
        REQUIRE(owned_node_count == 0);
    }
}

std::shared_ptr<inf::MeshInterface> createVolumeCartMeshPartitionedInTheMiddle(MessagePasser mp) {
    std::shared_ptr<inf::TinfMesh> cube = nullptr;
    if (mp.Rank() == 0)
        cube = inf::CartMesh::createVolume(4, 4, 1, {{0, 0, 0}, {1, 1, .25}});
    else
        cube = std::make_shared<inf::TinfMesh>(inf::TinfMeshData(), mp.Rank());
    std::vector<int> cell_owners(cube->cellCount(), -1);
    if (mp.Rank() == 0) {
        for (int c = 0; c < cube->cellCount(); c++) {
            auto cell = inf::Cell(*cube, c);
            if (cell.averageCenter()[0] < 0.5)
                cell_owners[c] = 0;
            else
                cell_owners[c] = 1;
        }
    }

    return inf::MeshShuffle::shuffleCells(mp, *cube, cell_owners);
}

int pickAFringeCell(const inf::MeshInterface& mesh) {
    auto c2c = inf::CellToCell::build(mesh);
    std::set<int> fringe_cells;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.ownedCell(c)) {
            for (auto neighbor : c2c[c]) {
                if (mesh.cellOwner(neighbor) != mesh.partitionId()) return neighbor;
            }
        }
    }
    PARFAIT_THROW("Could not find a fringe cell");
}

void visualizeOwners(std::string filename, MessagePasser mp, std::shared_ptr<inf::MeshInterface> mesh) {
    auto owners = inf::extractCellOwners(*mesh);
    std::vector<double> owners_double = Parfait::VectorTools::to_double(owners);

    inf::shortcut::visualize_at_cells(filename, mp, mesh, {owners_double}, {"cell-owner"});
}

TEST_CASE("Can sync a test mesh?") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto mesh = createVolumeCartMeshPartitionedInTheMiddle(mp);
    auto owners = inf::extractCellOwners(*mesh);
    auto cell_sync_pattern = inf::buildCellSyncPattern(mp, *mesh);
    std::vector<int> mock_field(mesh->cellCount(), mp.Rank());

    auto g2l_cell = inf::GlobalToLocal::buildCell(*mesh);

    Parfait::syncVector(mp, mock_field, g2l_cell, cell_sync_pattern);

    REQUIRE(mesh->cellCount() == 12);
}

TEST_CASE("Shuffle a mesh where the part vector is not synced in parallel", "[shuffle]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto mesh = createVolumeCartMeshPartitionedInTheMiddle(mp);

    REQUIRE(mesh->cellCount() == 12);

    std::vector<int> new_cell_owners(mesh->cellCount(), mp.Rank());
    for(int c = 0; c < mesh->cellCount(); c++){
        // explicitly mess up the part vector for non-owned cells
        if(not mesh->ownedCell(c)) {
            new_cell_owners[c] = -1;
        }
    }
    if (mp.Rank() == 0) {
        int fringe_cell = pickAFringeCell(*mesh);
        // move a fringe cell to the next partition
        new_cell_owners[fringe_cell] = 1;
    }
    auto shuffled_mesh = inf::MeshShuffle::shuffleCells(mp, *mesh, new_cell_owners);
}

TEST_CASE("Shuffle a mesh to make a fringe cell an interior cell", "[claim]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    auto mesh = createVolumeCartMeshPartitionedInTheMiddle(mp);
    REQUIRE(mesh->cellCount() == 12);

    visualizeOwners("claim-cell-init", mp, mesh);

    std::set<int> claimed_cells;
    long claimed_cell_gid = -1;
    if (mp.Rank() == 0) {
        int fringe_cell = pickAFringeCell(*mesh);
        claimed_cell_gid = mesh->globalCellId(fringe_cell);
        printf("Claiming fringe cell %d global %ld owned by %d\n", fringe_cell, claimed_cell_gid, mesh->cellOwner(fringe_cell));
        claimed_cells.insert(fringe_cell);
    }

    auto shuffled_mesh = inf::MeshShuffle::claimCellStencils(mp, *mesh, claimed_cells);
    REQUIRE(shuffled_mesh != nullptr);
    visualizeOwners("claim-cell-shuff", mp, shuffled_mesh);

    if(mp.Rank() == 0) {
        auto g2l_cell = inf::GlobalToLocal::buildCell(*shuffled_mesh);
        auto c2c = inf::CellToCell::build(*shuffled_mesh);
        int claimed_cell_local_id = g2l_cell.at(claimed_cell_gid);
        for (auto& neighbor : c2c[claimed_cell_local_id]) {
            REQUIRE(shuffled_mesh->ownedCell(neighbor));
        }
    }
    if (mp.Rank() == 0) REQUIRE(shuffled_mesh->countOwnedCells() == 12);
    if (mp.Rank() == 1) REQUIRE(shuffled_mesh->countOwnedCells() == 4);

}

TEST_CASE("Shuffle a partitioned mesh mostly to root") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() == 1) return;

    std::string mesh_filename = std::string(GRIDS_DIR) + "lb8.ugrid/5x5x5.lb8.ugrid";
    auto imported_mesh = inf::shortcut::loadMesh(mp, mesh_filename);
    std::shared_ptr<inf::TinfMesh> mesh = std::dynamic_pointer_cast<inf::TinfMesh>(imported_mesh);
    mesh->setTagName(1, "pikachu");
    REQUIRE(mesh->tagName(1) == "pikachu");


    std::vector<int> new_node_owners(mesh->nodeCount(), 0);
    if (mp.Rank() == 0) {
        for (int i = 0; i < mesh->nodeCount(); i++) {
            if (mesh->ownedNode(i)) {
                new_node_owners[i] = 1;
                break;
            }
        }
    }
    auto shuffled_mesh = inf::MeshShuffle::shuffleNodes(mp, *mesh, new_node_owners);
    REQUIRE(shuffled_mesh != nullptr);

    int owned_node_count = 0;
    for (int n = 0; n < shuffled_mesh->nodeCount(); n++) {
        if (shuffled_mesh->partitionId() == shuffled_mesh->nodeOwner(n)) {
            owned_node_count++;
        }
    }
    if (mp.Rank() == 0) {
        REQUIRE(owned_node_count > 2);
    } else if (mp.Rank() == 1) {
        REQUIRE(owned_node_count == 1);
    } else {
        REQUIRE(owned_node_count == 0);
    }
    REQUIRE(shuffled_mesh->tagName(1) == "pikachu");
}