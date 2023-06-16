#include <RingAssertions.h>
#include <map>
#include "MockMeshes.h"
#include <parfait/ExtentBuilder.h>
#include <parfait/VectorTools.h>
#include <t-infinity/Periodicity.h>
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/MeshPrinter.h>
#include <t-infinity/Extract.h>
#include <t-infinity/Shortcuts.h>

using PeriodicType = inf::Periodic::Type;

TEST_CASE("Periodic can find paired global node ids") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;
    auto mesh = inf::mock::getSingleHexMeshWithFaces(0);

    int bottom_tag = 1;
    int top_tag = 6;
    std::vector<std::pair<int, int>> tag_pairs = {{bottom_tag, top_tag}};
    auto periodicity = inf::Periodic::Matcher(mp, PeriodicType::TRANSLATION, *mesh, tag_pairs);
    REQUIRE(periodicity.tag_average_centroids.at(bottom_tag).approxEqual({0.5, 0.5, 0.0}));
    REQUIRE(periodicity.global_node_to_periodic_nodes.size() == 8);

    REQUIRE(periodicity.global_node_to_periodic_nodes.at(0).size() == 1);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(0).front().host_global_node_id == 0);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(0).front().virtual_global_node_id == 4);

    REQUIRE(periodicity.global_node_to_periodic_nodes.at(1).size() == 1);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(1).front().host_global_node_id == 1);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(1).front().virtual_global_node_id == 5);

    REQUIRE(periodicity.global_node_to_periodic_nodes.at(2).size() == 1);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(2).front().host_global_node_id == 2);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(2).front().virtual_global_node_id == 6);

    REQUIRE(periodicity.global_node_to_periodic_nodes.at(3).size() == 1);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(3).front().host_global_node_id == 3);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(3).front().virtual_global_node_id == 7);

    REQUIRE(periodicity.global_node_to_periodic_nodes.at(4).size() == 1);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(4).front().host_global_node_id == 4);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(4).front().virtual_global_node_id == 0);

    REQUIRE(periodicity.global_node_to_periodic_nodes.at(5).size() == 1);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(5).front().host_global_node_id == 5);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(5).front().virtual_global_node_id == 1);

    REQUIRE(periodicity.global_node_to_periodic_nodes.at(6).size() == 1);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(6).front().host_global_node_id == 6);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(6).front().virtual_global_node_id == 2);

    REQUIRE(periodicity.global_node_to_periodic_nodes.at(7).size() == 1);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(7).front().host_global_node_id == 7);
    REQUIRE(periodicity.global_node_to_periodic_nodes.at(7).front().virtual_global_node_id == 3);

    // periodic cells around node

    // periodic face neighbors
    auto g2l_cell = inf::GlobalToLocal::buildCell(*mesh);
    REQUIRE(periodicity.faces.size() == 2);
    bool found_bottom = false;
    bool found_top = false;

    for (auto& pair : periodicity.faces) {
        auto f = pair.second;
        REQUIRE(f.host_face_global_id == pair.first);
        REQUIRE(f.host_volume_global_id == 0);
        CHECK(f.owner_of_virtual_volume == 0);
        CHECK(f.owner_of_virtual_face == 0);
        CHECK(f.owner_of_host_volume == 0);
        CHECK(f.owner_of_host_face == 0);

        int local_cell_id = g2l_cell.at(f.host_face_global_id);
        if (mesh->cellTag(local_cell_id) == top_tag) {
            found_top = true;
            CHECK(f.host_face_global_id == 6);

            auto face_nodes = f.host_face_nodes;
            std::sort(face_nodes.begin(), face_nodes.end());
            CHECK(face_nodes == std::vector<long>{4, 5, 6, 7});
            face_nodes = f.virtual_face_nodes;
            std::sort(face_nodes.begin(), face_nodes.end());
            CHECK(face_nodes == std::vector<long>{0, 1, 2, 3});
        }
        if (mesh->cellTag(local_cell_id) == bottom_tag) {
            found_bottom = true;
            CHECK(f.host_face_global_id == 1);

            auto face_nodes = f.host_face_nodes;
            std::sort(face_nodes.begin(), face_nodes.end());
            CHECK(face_nodes == std::vector<long>{0, 1, 2, 3});
            face_nodes = f.virtual_face_nodes;
            std::sort(face_nodes.begin(), face_nodes.end());
            CHECK(face_nodes == std::vector<long>{4, 5, 6, 7});
        }
    }

    REQUIRE(found_top);
    REQUIRE(found_bottom);

    //    REQUIRE(periodicity.node_to_cell_including_periodic.size() == 8);
    // Make sure the two periodic faces aren't in the list.
    // only non-periodic faces and the volume cells
}

TEST_CASE("larger mesh search periodicity") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;
    auto mesh = inf::CartMesh::create(2, 2, 2);

    int bottom_tag = 1;
    int top_tag = 6;
    std::vector<std::pair<int, int>> tag_pairs = {{bottom_tag, top_tag}};
    auto periodicity = inf::Periodic::Matcher(mp, PeriodicType::TRANSLATION, *mesh, tag_pairs);

    REQUIRE(periodicity.global_node_to_periodic_nodes.size() == 18);
}

TEST_CASE("Does an empty extent intersect anything?") {
    auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
    Parfait::Point<double> p = {0, 0, 0};
    REQUIRE(not e.intersects(p));
}

TEST_CASE("Periodic works in mpi parallel", "[periodic]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() < 2) return;
    auto mesh = inf::CartMesh::create(mp, 10, 5, 1, {{0, 0, 0}, {1, .5, .1}});
    auto tags = inf::extractTags(*mesh);
    std::vector<std::pair<int, int>> tag_pairs = {{2, 4}};
    auto periodicity = inf::Periodic::Matcher(mp, PeriodicType::TRANSLATION, *mesh, tag_pairs);
}

TEST_CASE("Periodic translation point mover") {
    Parfait::Point<double> from = {0, 0, 0};
    Parfait::Point<double> to = {1, 0, 0};
    auto mover = inf::Periodic::createMover(PeriodicType::TRANSLATION, from, to);
    REQUIRE(mover({0, .25, .25}).approxEqual({1.0, .25, .25}));
}

TEST_CASE("Periodic x-symmetry point mover") {
    Parfait::Point<double> from = {.5, 1, 0};
    Parfait::Point<double> to = {0.5, 0, 1};
    auto mover = inf::Periodic::createMover(PeriodicType::AXISYMMETRIC_X, from, to);
    REQUIRE(mover({0.25, 11.9, 0}).approxEqual({.25, 0, 11.9}));
}

TEST_CASE("Periodic x-symmetry point mover reverse") {
    Parfait::Point<double> to = {.5, 1, 0};
    Parfait::Point<double> from = {0.5, 0, 1};
    printf("from %s, to %s\n", from.to_string().c_str(), to.to_string().c_str());
    auto mover = inf::Periodic::createMover(PeriodicType::AXISYMMETRIC_X, from, to);
    REQUIRE(mover({0.25, 0.0, 11.9}).approxEqual({.25, 11.9, 0.0}));
}

TEST_CASE("Periodic y-symmetry point mover") {
    Parfait::Point<double> from = {0, 1, 1};
    Parfait::Point<double> to = {1, 1, 0};
    auto mover = inf::Periodic::createMover(PeriodicType::AXISYMMETRIC_Y, from, to);
    REQUIRE(mover({0.0, 11.9, .25}).approxEqual({.25, 11.9, 0.0}));
}

TEST_CASE("Periodic y-symmetry point mover reverse") {
    Parfait::Point<double> to = {0, 1, 1};
    Parfait::Point<double> from = {1, 1, 0};
    auto mover = inf::Periodic::createMover(PeriodicType::AXISYMMETRIC_Y, from, to);
    REQUIRE(mover({0.25, 11.9, 0.0}).approxEqual({0.0, 11.9, 0.25}));
}

TEST_CASE("Periodic z-symmetry point mover") {
    Parfait::Point<double> from = {1, 0, 2};
    Parfait::Point<double> to = {0, 1, 2};
    auto mover = inf::Periodic::createMover(PeriodicType::AXISYMMETRIC_Z, from, to);
    REQUIRE(mover({11.7, 0.0, .25}).approxEqual({0.0, 11.7, .25}));
}

TEST_CASE("Periodic z-symmetry point mover reverse") {
    Parfait::Point<double> to = {1, 0, 2};
    Parfait::Point<double> from = {0, 1, 2};
    auto mover = inf::Periodic::createMover(PeriodicType::AXISYMMETRIC_Z, from, to);
    REQUIRE(mover({0.0, 11.7, .25}).approxEqual({11.7, 0.0, .25}));
}

TEST_CASE("Can ask for an axisymmetric periodicity") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;
    auto mesh = inf::CartMesh::create(2, 2, 2);
    int bottom_tag = 1;
    int front_tag = 3;
    std::vector<std::pair<int, int>> tag_pairs = {{bottom_tag, front_tag}};
    auto matcher = inf::Periodic::Matcher(mp, PeriodicType::AXISYMMETRIC_X, *mesh, tag_pairs);
    inf::MeshPrinter::printSurfaceCellNodes(mp, *mesh);
    REQUIRE(matcher.global_node_to_periodic_nodes.size() == 15);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(0).size() == 1);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(0)[0].host_global_node_id == 0);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(0)[0].virtual_global_node_id == 0);

    REQUIRE(matcher.global_node_to_periodic_nodes.at(1).size() == 1);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(1)[0].host_global_node_id == 1);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(1)[0].virtual_global_node_id == 1);

    REQUIRE(matcher.global_node_to_periodic_nodes.at(2).size() == 1);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(2)[0].host_global_node_id == 2);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(2)[0].virtual_global_node_id == 2);

    REQUIRE(matcher.global_node_to_periodic_nodes.at(3).size() == 1);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(3)[0].host_global_node_id == 3);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(3)[0].virtual_global_node_id == 9);

    REQUIRE(matcher.global_node_to_periodic_nodes.at(9).size() == 1);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(9)[0].host_global_node_id == 9);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(9)[0].virtual_global_node_id == 3);

    REQUIRE(matcher.global_node_to_periodic_nodes.at(20).size() == 1);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(20)[0].host_global_node_id == 20);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(20)[0].virtual_global_node_id == 8);

    REQUIRE(matcher.global_node_to_periodic_nodes.at(8).size() == 1);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(8)[0].host_global_node_id == 8);
    REQUIRE(matcher.global_node_to_periodic_nodes.at(8)[0].virtual_global_node_id == 20);
}