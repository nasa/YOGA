#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/MeshBuilder.h>
#include <t-infinity/MeshExtruder.h>
#include <parfait/Normals.h>

TEST_CASE("Extrude can have specific control over each layer added") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto surface = inf::CartMesh::createSphereSurface(5, 5, 5);
    auto builder = inf::experimental::MeshBuilder(mp, surface);

    double dx = 0.1;
    inf::extrude::extrudeTags(builder, {1}, false, false, dx, -98);
    inf::extrude::extrudeTags(builder, {-98}, true, false, dx, -98);
    inf::extrude::extrudeTags(builder, {-98}, true, false, dx, -98);
    inf::extrude::extrudeTags(builder, {-98}, true, false, dx, -98);
    inf::extrude::extrudeTags(builder, {-98}, true, false, dx, -98);
    inf::extrude::extrudeTags(builder, {1}, true, true, dx, -98);

    REQUIRE(builder.mesh->cellCount(inf::MeshInterface::HEXA_8) == 25 * 6);
    REQUIRE(builder.mesh->cellCount(inf::MeshInterface::QUAD_4) == 25 * 7);

    inf::shortcut::visualize("sphere-extrude", mp, builder.mesh);
}

TEST_CASE("Extrude can simply extrude a few layers easily") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto surface = inf::CartMesh::createSphereSurface(5, 5, 5);

    auto mesh = inf::extrude::extrudeTags(*surface, {1}, 0.1, 7, true);
    inf::shortcut::visualize("sphere-extrude-both", mp, mesh);
    REQUIRE(mesh->cellCount(inf::MeshInterface::HEXA_8) == 25 * 14);
}
