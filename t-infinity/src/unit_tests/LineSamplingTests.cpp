#include <RingAssertions.h>
#include <parfait/PointWriter.h>
#include <t-infinity/LineSampling.h>
#include <t-infinity/CartMesh.h>
#include <parfait/Adt3dExtent.h>
#include <t-infinity/MeshExtents.h>
#include <t-infinity/Shortcuts.h>
#include "MockFields.h"
#include <parfait/Timing.h>

TEST_CASE("Can create a line sampling cut for a mesh - Normal case") {
    auto mesh = inf::CartMesh::create(10, 10, 10);
    Parfait::Point<double> a = {-0.01, 0.51, 0.52};
    Parfait::Point<double> b = {1.01, 0.51, 0.52};

    auto cut = inf::linesampling::Cut(*mesh, a, b);
    auto points = cut.getSampledPoints();
    Parfait::PointWriter::write("sampled_points", points);
    REQUIRE(11 == cut.nodeCount());
}
TEST_CASE("Can create a line sampling cut for a mesh - Normal case with pre-built face neighbors") {
    auto mesh = inf::CartMesh::create(10, 10, 10);
    auto face_neighbors = inf::FaceNeighbors::build(*mesh);
    Parfait::Point<double> a = {-0.01, 0.51, 0.52};
    Parfait::Point<double> b = {1.01, 0.51, 0.52};

    auto cut = inf::linesampling::Cut(*mesh, face_neighbors, a, b);
    auto points = cut.getSampledPoints();
    Parfait::PointWriter::write("sampled_points", points);
    REQUIRE(11 == cut.nodeCount());
}
TEST_CASE("Can create a line sampling cut for a mesh - Glancing Face edges") {
    auto mesh = inf::CartMesh::create(10, 10, 10);
    Parfait::Point<double> a = {-0.01, 0.1, 0.52};
    Parfait::Point<double> b = {1.01, 0.1, 0.52};

    auto cut = inf::linesampling::Cut(*mesh, a, b);
    auto points = cut.getSampledPoints();
    Parfait::PointWriter::write("sampled_points", points);
    REQUIRE(22 == cut.nodeCount());
}
TEST_CASE("Can create a line sampling cut for a mesh - Diagonal Line") {
    auto mesh = inf::CartMesh::create(10, 10, 10);
    Parfait::Point<double> a = {-0.01, 0.09, 0.52};
    Parfait::Point<double> b = {1.01, 0.52, 0.09};

    auto cut = inf::linesampling::Cut(*mesh, a, b);
    auto points = cut.getSampledPoints();
    std::cout << points.size() << '\n';
    Parfait::PointWriter::write("sampled_points", points);
    REQUIRE(22 == cut.nodeCount());
}
TEST_CASE("Can create a dataframe from a sample - Normal Case") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    Parfait::Point<double> a = {-0.01, 0.1001, 0.52};
    Parfait::Point<double> b = {1.01, 0.1001, 0.52};
    auto test_field =
        inf::test::fillFieldAtNodes(*mesh, [](double x, double y, double z) { return 3.0 * x; });

    Parfait::DataFrame df = inf::linesampling::sample(mp, *mesh, a, b, {test_field}, {"test"});
    if(mp.Rank() == 0) {
        REQUIRE(df.columns().size() == 4);
        REQUIRE(df["x"].size() == 11);
        auto sampled_field = df["test"];
        REQUIRE(sampled_field.size() == 11);
    } else {
        REQUIRE(df.columns().size() == 0);
    }
}
TEST_CASE("Can create a dataframe from a sample - Normal Case with pre-built face neighbors") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    auto face_neighbors = inf::FaceNeighbors::build(*mesh);
    Parfait::Point<double> a = {-0.01, 0.1001, 0.52};
    Parfait::Point<double> b = {1.01, 0.1001, 0.52};
    auto test_field =
        inf::test::fillFieldAtNodes(*mesh, [](double x, double y, double z) { return 3.0 * x; });

    Parfait::DataFrame df = inf::linesampling::sample(mp, *mesh, face_neighbors, a, b, {test_field}, {"test"});
    if(mp.Rank() == 0) {
        REQUIRE(df.columns().size() == 4);
        REQUIRE(df["x"].size() == 11);
        auto sampled_field = df["test"];
        REQUIRE(sampled_field.size() == 11);
    } else {
        REQUIRE(df.columns().size() == 0);
    }
}
TEST_CASE("Can create a dataframe from a sample - Glancing Face Edges") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    Parfait::Point<double> a = {-0.01, 0.1, 0.52};
    Parfait::Point<double> b = {1.01, 0.1, 0.52};
    auto test_field =
        inf::test::fillFieldAtNodes(*mesh, [](double x, double y, double z) { return 3.0 * x; });

    Parfait::DataFrame df = inf::linesampling::sample(mp, *mesh, a, b, {test_field}, {"test"});
    if(mp.Rank() == 0) {
        REQUIRE(df.columns().size() == 4);
        REQUIRE(df["x"].size() == 22);
        auto sampled_field = df["test"];
        REQUIRE(sampled_field.size() == 22);
    } else {
        REQUIRE(df.columns().size() == 0);
    }
}
TEST_CASE("Can create a dataframe from a sample - Diagonal Line") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    Parfait::Point<double> a = {-0.01, 0.09, 0.52};
    Parfait::Point<double> b = {1.01, 0.52, 0.09};
    auto test_field =
        inf::test::fillFieldAtNodes(*mesh, [](double x, double y, double z) { return 3.0 * x; });

    Parfait::DataFrame df = inf::linesampling::sample(mp, *mesh, a, b, {test_field}, {"test"});
    if(mp.Rank() == 0) {
        REQUIRE(df.columns().size() == 4);
        REQUIRE(df["x"].size() == 22);
        auto sampled_field = df["test"];
        REQUIRE(sampled_field.size() == 22);
    } else {
        REQUIRE(df.columns().size() == 0);
    }
}
TEST_CASE("ADT accelerated cut returns the same points") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    auto face_neighbors = inf::FaceNeighbors::build(*mesh);
    auto overall_domain = inf::meshExtent(mp, *mesh);
    auto partition_domain = inf::partitionExtent(*mesh);
    Parfait::Adt3DExtent adt(partition_domain);
    for (int c = 0; c < mesh->cellCount(); c++) {
        auto cell_extent = Parfait::ExtentBuilder::buildExtentForCellInMesh(*mesh, c);
        adt.store(c, cell_extent);
    }
    // set the a and be to the farthest apart domain extent corners for worst case
    Parfait::Point<double> a, b;
    auto longest_distance = std::numeric_limits<double>::min();
    for (const auto& c1 : overall_domain.corners()) {
        for (const auto& c2 : overall_domain.corners()) {
            auto dist = c1.distance(c2);
            if (dist > longest_distance) {
                longest_distance = dist;
                a = c1;
                b = c2;
            }
        }
    }
    auto test_field = 
        inf::test::fillFieldAtNodes(*mesh, [](double x, double y, double z) { return 3.0 * x; });

    // normal cut
    auto start_time_standard = Parfait::Now();
    auto df_standard = inf::linesampling::sample(mp, *mesh, face_neighbors, a, b, {test_field}, {"test"});
    auto end_time_standard = Parfait::Now();
    auto df_adt = inf::linesampling::sample(mp, *mesh, face_neighbors, adt, a, b, {test_field}, {"test"});
    auto end_time_adt = Parfait::Now();
    INFO("Standard sample took " + Parfait::readableElapsedTimeAsString(start_time_standard, end_time_standard) + " s\n");
    INFO("Adt sample took " + Parfait::readableElapsedTimeAsString(end_time_standard, end_time_adt) + " s\n");

    if (mp.Rank() == 0) {
        REQUIRE(df_adt.columns().size() == 4);
        REQUIRE(df_standard["x"].size() == df_adt["x"].size());
        auto sampled_field_standard = df_standard["test"];
        auto sampled_field_adt = df_adt["test"];
        REQUIRE(sampled_field_standard.size() == sampled_field_adt.size());
    } else {
        REQUIRE(df_standard.columns().size() == 0);
        REQUIRE(df_adt.columns().size() == 0);
    }
}