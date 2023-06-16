#include <t-infinity/MeshInterface.h>
#include <MessagePasser/MessagePasser.h>
#include <map>
#include "../NodeCenteredPreProcessor/CellRedistributor.h"
#include <RingAssertions.h>

TEST_CASE("Cell Redistributor") {
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    std::map<inf::MeshInterface::CellType, std::vector<long>> old_cells;
    old_cells[inf::MeshInterface::CellType::TETRA_4] = std::vector<long>{0, 1, 2, 3};
    std::map<inf::MeshInterface::CellType, std::vector<long>> global_cell_ids;
    global_cell_ids[inf::MeshInterface::CellType::TETRA_4] = {100};

    std::vector<long> needed_nodes;
    if (mp.Rank() == 0) needed_nodes = std::vector<long>{0, 1};
    if (mp.Rank() == 1) needed_nodes = std::vector<long>{2, 3};
    std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
    cell_tags[inf::MeshInterface::CellType::TETRA_4].push_back(0);

    CellRedistributor cellRedistributor(mp, old_cells, cell_tags, global_cell_ids);

    auto queued_cells = cellRedistributor.queueCellsContainingNodes(needed_nodes);
    REQUIRE(queued_cells.size() == 1);
    REQUIRE(queued_cells[0].first == inf::MeshInterface::CellType::TETRA_4);
    REQUIRE(queued_cells[0].second == 0);
}

TEST_CASE("pack/unpack cells") {
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    std::map<inf::MeshInterface::CellType, std::vector<long>> old_cells;
    old_cells[inf::MeshInterface::CellType::TETRA_4] = std::vector<long>{0, 1, 2, 3};
    std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
    cell_tags[inf::MeshInterface::CellType::TETRA_4].push_back(0);
    std::map<inf::MeshInterface::CellType, std::vector<long>> global_cell_ids;
    global_cell_ids[inf::MeshInterface::CellType::TETRA_4] = {100};
    CellRedistributor cellRedistributor(mp, old_cells, cell_tags, global_cell_ids);

    std::vector<std::pair<inf::MeshInterface::CellType, int>> queued_cells = {
        {inf::MeshInterface::CellType::TETRA_4, 0}};
    std::vector<long> packed_cells = cellRedistributor.packCells(queued_cells);
    REQUIRE(packed_cells.size() == 7);

    CellCollection collection = cellRedistributor.unpackCells({packed_cells});
    REQUIRE(collection.cells.size() == 1);
    REQUIRE(collection.cell_tags.size() == 1);
    REQUIRE(collection.cell_global_ids.size() == 1);
}