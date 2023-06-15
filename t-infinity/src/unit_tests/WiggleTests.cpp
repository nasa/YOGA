#include <RingAssertions.h>
#include <t-infinity/Wiggle.h>
#include <t-infinity/CartMesh.h>

TEST_CASE("Can wiggle nodes in parallel", "[wiggle]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 5, 5, 5);

    double edge_length_percent = 10;
    inf::wiggleNodes(mp, *mesh, edge_length_percent);

    auto g2l = inf::GlobalToLocal::buildNode(*mesh);

    for (long gid = 0; gid < 215; gid++) {
        int owner = -1;  // everyone says they don't own it
        Parfait::Point<double> p = {-1, -1, -1};
        if (g2l.count(gid) == 1) {
            // you at least have this node resident.
            auto lid = g2l.at(gid);
            if (mesh->nodeOwner(lid) == mesh->partitionId()) {
                owner = mesh->partitionId();  // the owner says I own it
                p = mesh->node(lid);
            }
        }
        owner = mp.ParallelMax(owner);
        mp.Broadcast(p, owner);

        // if you have this node resident it better have the same xyz value
        if (g2l.count(gid) == 1) {
            auto lid = g2l.at(gid);
            Parfait::Point<double> q = mesh->node(lid);
            CHECK((p - q).magnitude() < 1e-12);
        }
    }
}