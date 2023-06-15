#include <RingAssertions.h>
#include <memory>
#include <MessagePasser/MessagePasser.h>
#include <parfait/Extent.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/MeshConnectivity.h>
#include <parfait/GraphColoring.h>

TEST_CASE("Coloring sync pattern tests") {
    MessagePasser mp(MPI_COMM_WORLD);
    int n = 5;
    Parfait::Extent<double> e{{0, 0, 0}, {1, 1, 1}};
    auto mesh = inf::CartMesh::create(mp, n, n, n, e);

    auto n2n = inf::NodeToNode::build(*mesh);

    std::vector<long> gids(mesh->nodeCount());
    std::vector<int> owners(mesh->nodeCount());
    for (int i = 0; i < mesh->nodeCount(); i++) {
        gids[i] = mesh->globalNodeId(i);
        owners[i] = mesh->nodeOwner(i);
    }

    auto colors = Parfait::GraphColoring::color(mp, n2n, gids, owners);

    auto color_sync_patterns = Parfait::GraphColoring::makeSyncPatternsForEachColor(mp, colors, gids, owners);
    auto g2l = Parfait::MapTools::invert(gids);

    auto test_field = [](double x, double y, double z) { return 3 * x + 4 * y - 2 * z; };

    std::vector<double> linear_field(mesh->nodeCount(), -99.998);
    for (int i = 0; i < mesh->nodeCount(); i++) {
        if (mesh->nodeOwner(i) == mp.Rank()) {
            auto p = mesh->node(i);
            linear_field[i] = test_field(p[0], p[1], p[2]);
        }
    }

    //--- sync the first color
    auto& syncer_1 = color_sync_patterns.at(1);
    Parfait::syncVector(mp,linear_field, g2l, syncer_1);

    //--- make sure ghosts of the first color are correct, but the rest of the ghosts aren't
    for (int i = 0; i < mesh->nodeCount(); i++) {
        auto p = mesh->node(i);
        if (colors[i] == 1) {
            REQUIRE(linear_field[i] == test_field(p[0], p[1], p[2]));
        } else if (mesh->nodeOwner(i) != mp.Rank()) {
            REQUIRE(linear_field[i] == -99.998);
        }
    }

    auto& syncer_2 = color_sync_patterns.at(2);
    Parfait::syncVector(mp,linear_field, g2l, syncer_2);
    //--- make sure ghosts of the first two colors are correct, but the rest of the ghosts aren't
    for (int i = 0; i < mesh->nodeCount(); i++) {
        auto p = mesh->node(i);
        if (colors[i] == 2 or colors[i] == 1) {
            REQUIRE(linear_field[i] == test_field(p[0], p[1], p[2]));
        } else if (mesh->nodeOwner(i) != mp.Rank() ) {
            REQUIRE(linear_field[i] == -99.998);
        }
    }
}
