#include <RingAssertions.h>
#include <FloodFill.h>
#include <MessagePasser/MessagePasser.h>
#include <parfait/LinearPartitioner.h>
#include <parfait/SyncPattern.h>
#include <t-infinity/CartMesh.h>
#include <parfait/ExtentBuilder.h>
#include <t-infinity/GhostSyncer.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/FilterFactory.h>

using namespace inf;
using namespace YOGA;

TEST_CASE("flood fill in serial"){
    std::vector<std::vector<int>> node_neighbors{{1},{0}};
    FloodFill f(node_neighbors);

    int from = 1;
    int to = 2;
    std::map<int,int> transitions;
    transitions[from] = to;
    std::set<int> seeds;
    seeds.insert(0);

    SECTION("if no seeds match target value, values remain unchanged") {
        std::vector<int> node_values{0, 0};
        f.fill(node_values, seeds, transitions);
        std::vector<int> expected{0, 0};
        REQUIRE(expected == node_values);
    }

    SECTION("if seed matches, it should transitions to the new value"){
        std::vector<int> node_values{1, 0};
        f.fill(node_values, seeds, transitions);
        std::vector<int> expected{2, 0};
        REQUIRE(expected == node_values);
    }

    SECTION("if neighbor of seed matches, it should also transitions to the new value"){
        std::vector<int> node_values{1, 1};
        f.fill(node_values, seeds, transitions);
        std::vector<int> expected{2, 2};
        REQUIRE(expected == node_values);
    }
}

void paintCellsInExtent(std::vector<int>& cell_values,
                        const MeshInterface& mesh,
                        const Parfait::Extent<double>& e,
                        int value){
    std::vector<int> cell;
    for(int i=0;i<mesh.cellCount();i++){
        mesh.cell(i,cell);
        auto cell_extent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        for(int node_id:cell) {
            Parfait::Point<double> p;
            mesh.nodeCoordinate(node_id,p.data());
            Parfait::ExtentBuilder::add(cell_extent, p);
        }
        if(cell_extent.intersects(e))
            cell_values[i] = value;
    }
}

int count(MessagePasser mp, const MeshInterface& mesh, const std::vector<int>& v,int value){
    int n = 0;
    for(int i=0;i<mesh.cellCount();i++)
        if(mesh.ownedCell(i) and v[i] == value)
            n++;
    return mp.ParallelSum(n);
}

TEST_CASE("flood fill in parallel") {
    MessagePasser mp(MPI_COMM_WORLD);
    printf("create cartmesh");
    fflush(stdout);
    int N = 2;
    auto full_mesh = CartMesh::create(mp, N, N, N);
    auto filter = FilterFactory::volumeFilter(mp.getCommunicator(),full_mesh);
    auto mesh = filter->getMesh();
    printf("build c2c\n");
    fflush(stdout);
    auto cell_to_cell = CellToCell::build(*mesh);
    printf("Build syncer\n");
    fflush(stdout);
    GhostSyncer syncer(mp, mesh);
    syncer.initializeCellSyncing();

    SECTION("if all cells have the same initial value, they should all transition"){
    std::set<int> seeds;
    if (mp.Rank() == 0) seeds.insert(0);

    int from = 7;
    int to = 8;
    std::map<int,int> transitions;
    transitions[from] = to;

    std::vector<int> cell_values(mesh->cellCount(), 7);
    FloodFill floodFill(cell_to_cell);

    auto sync = [&]() { syncer.syncCells(cell_values); };
    auto parallel_max = [&](int n) { return mp.ParallelMax(n); };
    printf("flood fill\n");
    fflush(stdout);
    floodFill.fill(cell_values, seeds, transitions, sync, parallel_max);

    std::vector<int> exepected(mesh->cellCount(), 8);
    REQUIRE(exepected == cell_values);
    }

    SECTION("if a plane of cells are marked with a different initial value, only half of the cells should transition"){
        std::set<int> seeds;
        if (mp.Rank() == 0) seeds.insert(0);

        int from = 7;
        int to = 8;
        std::map<int,int> transitions;
        transitions[from] = to;

        std::vector<int> cell_values(mesh->cellCount(), 7);
        double x = .51;
        Parfait::Extent<double> center_plane {{x,0,0},{x,1,1}};
        Parfait::Extent<double> right_half {{x,0,0},{1,1,1}};
        paintCellsInExtent(cell_values,*mesh,center_plane,11);

        int expect_11s = count(mp,*mesh,cell_values,11);



        FloodFill floodFill(cell_to_cell);
        auto sync = [&]() { syncer.syncCells(cell_values); };
        auto parallel_max = [&](int n) { return mp.ParallelMax(n); };
        floodFill.fill(cell_values, seeds, transitions, sync, parallel_max);

        int actual_7s = count(mp,*mesh,cell_values,7);
        int actual_8s = count(mp,*mesh,cell_values,8);
        int actual_11s = count(mp,*mesh,cell_values,11);
        int expect_8s = mesh->cellCount()/2;
        int expect_7s = mesh->cellCount() - actual_8s - actual_11s;

        REQUIRE(expect_11s == actual_11s);
        REQUIRE(expect_8s == actual_8s);
        REQUIRE(expect_7s == actual_7s);
    }

    SECTION("if multiple transitions are defined, the flood fill should continue across different transition_from values"){
        std::set<int> seeds;
        if (mp.Rank() == 0) seeds.insert(0);

        int from = 7;
        int other_from = 9;
        int to = 8;
        std::map<int,int> transitions;
        transitions[from] = to;
        transitions[other_from] = to;

        std::vector<int> cell_values(mesh->cellCount(), 7);
        double x = .51;
        Parfait::Extent<double> center_plane {{x,0,0},{x,1,1}};
        paintCellsInExtent(cell_values,*mesh,center_plane,other_from);

        FloodFill floodFill(cell_to_cell);
        auto sync = [&]() { syncer.syncCells(cell_values); };
        auto parallel_max = [&](int n) { return mp.ParallelMax(n); };
        floodFill.fill(cell_values, seeds, transitions, sync, parallel_max);

        int actual_7s = count(mp,*mesh,cell_values,7);
        int actual_8s = count(mp,*mesh,cell_values,8);
        int actual_9s = count(mp,*mesh,cell_values,9);
        int expect_8s = mesh->cellCount();
        int expect_7s = 0;
        int expect_9s = 0;

        REQUIRE(expect_7s == actual_7s);
        REQUIRE(expect_8s == actual_8s);
        REQUIRE(expect_9s == actual_9s);
    }
}