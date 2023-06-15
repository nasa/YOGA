#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/MeshShuffle.h>

TEST_CASE("Can do 4D points") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::createSphere(mp, 20, 20, 20, {{-1, -1, -1}, {1, 1, 1}});
    std::vector<double> node_dist(mesh->nodeCount(), 0);
    Parfait::Point<double> origin(0, 0, 0);
    for (int n = 0; n < mesh->nodeCount(); n++) {
        Parfait::Point<double> p = mesh->node(n);
        node_dist[n] = origin.distance(p);
    }

    auto part = inf::MeshShuffle::repartitionNodes4D(mp, *mesh, node_dist);
    auto repartitioned_mesh = inf::MeshShuffle::shuffleNodes(mp, *mesh, part);
}