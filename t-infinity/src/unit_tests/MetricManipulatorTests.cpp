#include <RingAssertions.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/TinfMesh.h>
#include <t-infinity/MetricManipulator.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/MeshShard.h>
#include <t-infinity/MeshAdaptionInterface.h>
#include <t-infinity/PluginLocator.h>
#include "MockMeshes.h"
#include "t-infinity/MeshHelpers.h"

using namespace inf;

void print(const Parfait::DenseMatrix<double, 3, 3>& M) { M.print(); }

TEST_CASE("calc implied metric for mesh - 3D") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = mock::getSingleTetMesh(0);
    int dimensionality = 3;

    SECTION("at nodes") {
        auto metric = MetricManipulator::calcImpliedMetricAtNodes(*mesh, dimensionality);

        REQUIRE(metric->size() == mesh->nodeCount());

        double complexity = MetricManipulator::calcComplexity(mp, *mesh, *metric);
        REQUIRE(complexity > 0);
        double target_complexity = 2.0 * complexity;
        auto scaled = MetricManipulator::scaleToTargetComplexity(mp, *mesh, *metric, target_complexity);

        double scaled_complexity = MetricManipulator::calcComplexity(mp, *mesh, *scaled);
        REQUIRE(target_complexity == Approx(scaled_complexity));
    }
    SECTION("at cells") {
        auto metric = MetricManipulator::calcImpliedMetricAtCells(*mesh, dimensionality);
        REQUIRE(metric->size() == mesh->cellCount());

        double complexity = MetricManipulator::calcComplexity(mp, *mesh, *metric);
        REQUIRE(complexity > 0);

        SECTION("global scaling") {
            double target_complexity = 2.0 * complexity;
            auto scaled = MetricManipulator::scaleToTargetComplexity(mp, *mesh, *metric, target_complexity);

            double scaled_complexity = MetricManipulator::calcComplexity(mp, *mesh, *scaled);
            REQUIRE(target_complexity == Approx(scaled_complexity));
        }
    }
}

TEST_CASE("calc implied metric for mesh - 2D") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = std::make_shared<TinfMesh>(mock::triangleWithBar(), 0);
    int dimensionality = 2;

    SECTION("at nodes") {
        auto metric = MetricManipulator::calcImpliedMetricAtNodes(*mesh, dimensionality);

        REQUIRE(metric->size() == mesh->nodeCount());

        double complexity = MetricManipulator::calcComplexity(mp, *mesh, *metric);
        REQUIRE(complexity > 0);
        double target_complexity = 2.0 * complexity;
        auto scaled = MetricManipulator::scaleToTargetComplexity(mp, *mesh, *metric, target_complexity);

        double scaled_complexity = MetricManipulator::calcComplexity(mp, *mesh, *scaled);
        REQUIRE(target_complexity == Approx(scaled_complexity));
    }
    SECTION("at cells") {
        auto metric = MetricManipulator::calcImpliedMetricAtCells(*mesh, dimensionality);
        REQUIRE(metric->size() == mesh->cellCount());

        double complexity = MetricManipulator::calcComplexity(mp, *mesh, *metric);
        REQUIRE(complexity > 0);

        SECTION("global scaling") {
            double target_complexity = 2.0 * complexity;
            auto scaled = MetricManipulator::scaleToTargetComplexity(mp, *mesh, *metric, target_complexity);

            double scaled_complexity = MetricManipulator::calcComplexity(mp, *mesh, *scaled);
            REQUIRE(target_complexity == Approx(scaled_complexity));
        }
    }
}

double symmetricDeterminant(const Parfait::DenseMatrix<double, 3, 3>& m) {
    double d = calcMetricDensity(m);
    return d * d;
}

bool determinantMatches(Tensor m, double det, double tol = 1e-12) {
    double d = symmetricDeterminant(m);
    double difference = std::abs(det - d);
    if (difference < tol) return true;
    printf("%e != %e (tol: %e, difference: %e)\n", d, det, tol, difference);
    return false;
}

TEST_CASE("calc determinant of metric") {
    SECTION("zero matrix") {
        Tensor m{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
        REQUIRE(determinantMatches(m, 0.0));
    }
    SECTION("identity") {
        Tensor m{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        REQUIRE(determinantMatches(m, 1.0));
    }
    SECTION("diagonal matrix") {
        Tensor m{{10, 0, 0}, {0, 2, 0}, {0, 0, 5}};
        REQUIRE(determinantMatches(m, 100.0));
    }
    SECTION("another matrix") {
        Tensor m{{2, -1, 0}, {-1, 2, -1}, {0, -1, 2}};
        REQUIRE(determinantMatches(m, 4.0));
    }
    SECTION("really big values") {
        Tensor m{{3.677054186758148e+14, -1.485195394845776e+15, -1.208526355699555e+15},
                 {-1.485195394845776e+15, 5.998838665164314e+15, 4.881347299319029e+15},
                 {-1.208526355699555e+15, 4.881347299319029e+15, 3.972027399714484e+15}};
        double tol = 5.0e22;
        REQUIRE(determinantMatches(m, 9.38923919771131e+28, tol));
    }
}

TEST_CASE("do spacing stuff") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto cart_mesh = CartMesh::create(mp, 10, 10, 15);
    auto refine_shard = getMeshShard(getPluginDir(), "RefinePlugins");
    auto tet_mesh = refine_shard->shard(mp.getCommunicator(), *cart_mesh);
    REQUIRE(globalNodeCount(mp, *tet_mesh) == globalNodeCount(mp, *cart_mesh));
    REQUIRE(globalCellCount(mp, *tet_mesh) != globalCellCount(mp, *cart_mesh));

    int dimensionality = 3;

    auto metric = MetricManipulator::calcImpliedMetricAtNodes(*tet_mesh, dimensionality);
    REQUIRE(tet_mesh->nodeCount() == metric->size());
    REQUIRE(6 == metric->blockSize());

    auto complexity = MetricManipulator::calcComplexity(mp, *tet_mesh, *metric);
    printf("My complexity is %e\n", complexity);

    int zminus_tag = 1;
    double target_complexity = 4000.0;

    auto prescribed_metric =
        MetricManipulator::setPrescribedSpacingOnMetric(mp, *tet_mesh, *metric, {zminus_tag}, target_complexity, 0.01);

    REQUIRE(tet_mesh->nodeCount() == prescribed_metric->size());
    REQUIRE(6 == prescribed_metric->blockSize());

    auto new_complexity = MetricManipulator::calcComplexity(mp, *tet_mesh, *prescribed_metric);
    printf("the new complexity: %e\n", new_complexity);
    REQUIRE(new_complexity == Approx(target_complexity));

    //    auto ref_adapt = getMeshAdaptationPlugin(getPluginDir(), "RefinePlugins");
    //
    //    Parfait::Dictionary options;
    //    auto adapted_mesh = ref_adapt->adapt(tet_mesh, mp.getCommunicator(), prescribed_metric, options.dump());
    //
    //    shortcut::visualize("original", mp, tet_mesh);
    //    shortcut::visualize("adapted", mp, adapted_mesh);
}

TEST_CASE("Calculate implied complexity in parallel") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::shared_ptr<MeshInterface> mesh = CartMesh::create(mp, 10, 10, 10);
    auto refine_shard = getMeshShard(getPluginDir(), "RefinePlugins");
    mesh = refine_shard->shard(mp.getCommunicator(), *mesh);

    int dimensionality = 3;

    auto metric = MetricManipulator::calcImpliedMetricAtNodes(*mesh, dimensionality);
    auto complexity = MetricManipulator::calcComplexity(mp, *mesh, *metric);

    double expected_complexity = 7.071068e+02;
    REQUIRE(expected_complexity == Approx(complexity));
}
