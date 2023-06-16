#include <RingAssertions.h>
#include <array>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/ElevateMesh.h>
#include <t-infinity/SurfaceMeshReconstruction.h>

TEST_CASE("Can reconstruct a P2 surface from a P1 surface", "[surface-reconstruction]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::createSphereSurface(3, 3, 3);
    mesh = std::make_shared<inf::TinfMesh>(inf::shortcut::shard(mp, *mesh));
    inf::shortcut::visualize("p1_sphere", mp, mesh);

    std::vector<Parfait::Point<double>> cell_normals(mesh->cellCount());
    std::vector<Parfait::Point<double>> node_normals(mesh->nodeCount());

    for (int c = 0; c < mesh->cellCount(); c++) {
        inf::Cell cell(mesh, c);
        auto normal = cell.faceAreaNormal(0);
        normal.normalize();
        cell_normals[c] = normal;
    }

    auto n2c = inf::NodeToCell::build(*mesh);
    for (int n = 0; n < mesh->nodeCount(); n++) {
        for (auto c : n2c[n]) node_normals[n] += cell_normals[c];
        node_normals[n] *= 1.0 / (double)n2c[n].size();
    }

    auto p2_mesh = inf::elevateToP2(mp, *mesh, inf::P1ToP2::getMapQuadratic());

    inf::SurfaceReconstruction::curveMidEdgeNodesInTri6(*p2_mesh, node_normals);

    inf::shortcut::visualize("p2_sphere.3D", mp, p2_mesh);
    inf::shortcut::visualize("p2_sphere", mp, p2_mesh);
}