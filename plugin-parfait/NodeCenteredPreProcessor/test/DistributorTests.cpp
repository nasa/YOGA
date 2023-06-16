#include <../shared/UgridReader.h>
#include <t-infinity/MeshInterface.h>
#include <MessagePasser/MessagePasser.h>
#include "../Distributor.h"
#include "../shared/MockReaders.h"
#include <RingAssertions.h>

namespace {
std::string grids_dir = GRIDS_DIR;
std::string six_cell = grids_dir + "lb8.ugrid/6cell.lb8.ugrid";
}

using namespace inf;

TEST_CASE("Distributor Exists") {
    auto reader = std::make_shared<MockReader>();
    MessagePasser mp(MPI_COMM_WORLD);

    Distributor distributor(mp, reader);

    if (mp.NumberOfProcesses() != 2) return;
    std::vector<Parfait::Point<double>> coords = distributor.distributeNodes();
    REQUIRE(coords.size() == 2);

    std::vector<long> tets;
    std::vector<int> tags;
    std::vector<long> global_cell_ids;
    std::tie(tets, tags, global_cell_ids) =
        distributor.distributeCellsTagsAndGlobalIds2(MeshInterface::CellType::TETRA_4);
    REQUIRE(tets.size() == 4);
    REQUIRE(tags.size() == 1);
    REQUIRE(global_cell_ids.size() == 1);
}

TEST_CASE("Distributor node range") {
    auto reader = std::make_shared<MockReader>();
    MessagePasser mp(MPI_COMM_WORLD);

    Distributor distributor(mp, reader);

    if (mp.NumberOfProcesses() != 2) return;

    auto node_range = distributor.nodeRange();
    if (mp.Rank() == 0) {
        REQUIRE(node_range.start == 0);
        REQUIRE(node_range.end == 2);
    }
    if (mp.Rank() == 1) {
        REQUIRE(node_range.start == 2);
        REQUIRE(node_range.end == 4);
    }
}

TEST_CASE("cell types") {
    auto reader = std::make_shared<TwoTetReader>();
    MessagePasser mp(MPI_COMM_WORLD);

    Distributor distributor(mp, reader);
    std::vector<MeshInterface::CellType> cell_types = distributor.cellTypes();
    REQUIRE(cell_types.size() == 1);
}

TEST_CASE("Distribute two tets") {
    auto reader = std::make_shared<TwoTetReader>();
    MessagePasser mp(MPI_COMM_WORLD);

    Distributor distributor(mp, reader);

    if (mp.NumberOfProcesses() != 2) return;
    std::vector<Parfait::Point<double>> coords = distributor.distributeNodes();
    REQUIRE(coords.size() == 4);

    std::vector<long> tets;
    std::vector<int> tags;
    std::vector<long> global_cell_ids;
    std::tie(tets, tags, global_cell_ids) =
        distributor.distributeCellsTagsAndGlobalIds2(MeshInterface::CellType::TETRA_4);
    REQUIRE(tets.size() == 4);
    REQUIRE(tags.size() == 1);
    REQUIRE(tags[0] == mp.Rank());
    REQUIRE(global_cell_ids[0] == mp.Rank());
}

TEST_CASE("Distributor can export a NaiveMesh") {
    auto reader = std::make_shared<UgridReader>(six_cell);

    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    Distributor distributor(mp, reader);

    std::map<inf::MeshInterface::CellType, std::vector<long>> cells;
    std::map<inf::MeshInterface::CellType, std::vector<int>> cell_tags;
    std::map<inf::MeshInterface::CellType, std::vector<long>> global_cell_ids;
    auto cell_types = reader->cellTypes();
    for (auto type : cell_types) {
        std::tie(cells[type], cell_tags[type], global_cell_ids[type]) =
            distributor.distributeCellsTagsAndGlobalIds2(type);
    }
    auto range_per_rank = distributor.buildNodeRangePerRank(mp.NumberOfProcesses(), distributor.globalNodeCount());
    std::vector<long> node_rank_ownership_range;
    for (auto r : range_per_rank) {
        node_rank_ownership_range.push_back(r.start);
    }
    auto r = range_per_rank.back();
    node_rank_ownership_range.push_back(r.end);

    REQUIRE(cell_tags.at(MeshInterface::TRI_3).size() == 6);
    REQUIRE(cells.at(MeshInterface::TRI_3).size() == 6 * 3);
    REQUIRE(global_cell_ids.at(MeshInterface::TRI_3).size() == 6);
    REQUIRE(global_cell_ids.at(MeshInterface::PENTA_6).size() == 6);
    REQUIRE(global_cell_ids.at(MeshInterface::QUAD_4).size() == 6);
    REQUIRE(int(node_rank_ownership_range.size()) == mp.NumberOfProcesses() + 1);
    REQUIRE(node_rank_ownership_range[0] == 0);
    REQUIRE(node_rank_ownership_range[1] == 7);
    REQUIRE(node_rank_ownership_range[2] == 14);

    long max_id = 0;
    for (auto& row : global_cell_ids)
        for (long id : row.second) max_id = std::max(max_id, id);
    max_id = mp.ParallelMax(max_id);

    const long number_of_total_cells = 6 + 6 + 12;
    const long expected_max_global_cell_id = number_of_total_cells - 1;
    REQUIRE(expected_max_global_cell_id == max_id);
}
