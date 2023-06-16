#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/VisualizePartition.h>
#include <t-infinity/MeshMirror.h>
#include "t-infinity/FilterFactory.h"
#include "t-infinity/MeshHelpers.h"

using namespace inf;

void vizSurface(const std::string& name, const MessagePasser& mp, std::shared_ptr<MeshInterface> mesh) {
    auto surface_mesh = FilterFactory::surfaceFilter(mp.getCommunicator(), mesh)->getMesh();
    inf::visualizePartition(name, *surface_mesh, {});
}

using namespace inf;

TEST_CASE("Can mirror a mesh") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto half_mesh = inf::CartMesh::create(mp, 2, 2, 2);
    auto zipper = MeshMirror::mirrorViaTags(mp, half_mesh, {1});
    auto mirrored_once = zipper.getMesh();

    SECTION("mirror mesh one time") {
        REQUIRE(8 == globalCellCount(mp, *half_mesh, MeshInterface::HEXA_8));
        REQUIRE(16 == globalCellCount(mp, *mirrored_once, MeshInterface::HEXA_8));
        REQUIRE(24 == globalCellCount(mp, *half_mesh, MeshInterface::QUAD_4));
        REQUIRE(40 == globalCellCount(mp, *mirrored_once, MeshInterface::QUAD_4));
        REQUIRE(45 == globalNodeCount(mp, *mirrored_once));

        for(int i=0;i<half_mesh->nodeCount();i++){
            int orig_id = zipper.previousNodeId(i);
            Parfait::Point<double> original_xyz = half_mesh->node(orig_id);
            Parfait::Point<double> combined_xyz = mirrored_once->node(i);
            REQUIRE(orig_id == i);
            REQUIRE(original_xyz.approxEqual(combined_xyz));
        }

        for(int i=half_mesh->nodeCount();i<mirrored_once->nodeCount();i++){
            int orig_id = zipper.previousNodeId(i);
            REQUIRE(orig_id != i);
            Parfait::Point<double> original_xyz = half_mesh->node(orig_id);
            Parfait::Point<double> combined_xyz = mirrored_once->node(i);
            REQUIRE(original_xyz[0] == combined_xyz[0]);
            REQUIRE(original_xyz[1] == combined_xyz[1]);
            REQUIRE(-original_xyz[2] == combined_xyz[2]);
        }

    }

    SECTION("mirror again"){
        zipper = MeshMirror::mirrorViaTags(mp, mirrored_once, {1, 6});
        auto mirrored_twice = zipper.getMesh();

        REQUIRE(32 == globalCellCount(mp, *mirrored_twice, MeshInterface::HEXA_8));
        REQUIRE(64 == globalCellCount(mp, *mirrored_twice, MeshInterface::QUAD_4));
    }

    //    vizSurface("half_mesh", mp, half_mesh);
    //    vizSurface("first_mirror", mp, mirrored_once);
    //    vizSurface("mirrored_mesh", mp, mirrored_twice);



}