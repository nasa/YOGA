#include <RingAssertions.h>
#include <t-infinity/MeshQualityMetrics.h>
#include "MockMeshes.h"
#include <parfait/PointGenerator.h>
#include <t-infinity/MeshOptimization.h>
#include <t-infinity/Shortcuts.h>

TEST_CASE("calculate corner jacobian") {
    SECTION("Perfect corner") {
        Parfait::Point<double> e1 = {1, 0, 0};
        Parfait::Point<double> e2 = {0, 1, 0};
        Parfait::Point<double> e3 = {0, 0, 1};

        auto jacobian = Parfait::Point<double>::dot(e1, Parfait::Point<double>::cross(e2, e3));
        REQUIRE(jacobian == Approx(1.0));
    }

    SECTION("Perfectly inverted corner") {
        Parfait::Point<double> e1 = {1, 0, 0};
        Parfait::Point<double> e2 = {0, 1, 0};
        Parfait::Point<double> e3 = {0, 0, -1};

        auto jacobian = Parfait::Point<double>::dot(e1, Parfait::Point<double>::cross(e2, e3));
        REQUIRE(jacobian == Approx(-1.0));
    }

    SECTION("nearly flat corner") {
        Parfait::Point<double> e1 = {1, 0, 0};
        Parfait::Point<double> e2 = {0, 1, 0};
        Parfait::Point<double> e3 = {1, 1, 1e-2};
        e3.normalize();

        auto jacobian = Parfait::Point<double>::dot(e1, Parfait::Point<double>::cross(e2, e3));
        REQUIRE(jacobian == Approx(1e-2 * sqrt(2.0) / 2.0).margin(1.0e-5));
    }
}


TEST_CASE("Cost Quad") {
    using namespace inf;
    SECTION("Manually extracted poor element") {
        Parfait::Point<double> a = {.793, .109, 0};
        Parfait::Point<double> b = {.83, .109, 0};
        Parfait::Point<double> c = {.87, .116, 0};
        Parfait::Point<double> d = {.82, .15, 0};
        auto u = c - b;
        auto v = a - b;
        u.normalize();
        v.normalize();

        double angle = acos(u.dot(v))*180/M_PI;
        REQUIRE(angle > 170);
        auto cost = MeshQuality::cellHilbertCost(inf::MeshInterface::QUAD_4, {a, b, c, d});
        REQUIRE(cost > 0.8);

    }
    SECTION("ideal quad") {
        Parfait::Point<double> a = {0, 0, 0};
        Parfait::Point<double> b = {1, 0, 0};
        Parfait::Point<double> c = {1, 1, 0};
        Parfait::Point<double> d = {0, 1, 0};
        auto cost = MeshQuality::cellHilbertCost(inf::MeshInterface::QUAD_4, {a, b, c, d});
        REQUIRE(cost == Approx(0.0).margin(1e-8));
    }

    SECTION("flattened corner quad") {
        Parfait::Point<double> a = {0, 0, 0};
        Parfait::Point<double> b = {1, 0, 0};
        Parfait::Point<double> c = {2, 0.0, 0};
        Parfait::Point<double> d = {1, 1, 0};

        auto u = c - b;
        auto v = a - b;
        u.normalize();
        v.normalize();

        double angle = acos(u.dot(v))*180/M_PI;
        REQUIRE(angle > 175);
        auto cost = MeshQuality::cellHilbertCost(inf::MeshInterface::QUAD_4, {a, b, c, d});
        REQUIRE(cost > 1.0);
    }

}

TEST_CASE("Cost Triangle") {
    using namespace inf;

    SECTION("right triangle") {
        Parfait::Point<double> a = {0, 0, 0};
        Parfait::Point<double> b = {1, 0, 0};
        Parfait::Point<double> c = {1, 1, 0};
        auto cost = MeshQuality::cellHilbertCost(inf::MeshInterface::TRI_3, {a, b, c});
        REQUIRE(cost > 0.0);
        REQUIRE(cost < 1.0);
    }

    SECTION("flat triangle") {
        Parfait::Point<double> a = {0, 0, 0};
        Parfait::Point<double> b = {1, 0, 0};
        Parfait::Point<double> c = {1, 0, 0};
        auto cost = MeshQuality::cellHilbertCost(inf::MeshInterface::TRI_3, {a, b, c});
        REQUIRE(cost == Approx(1.0));
    }

    SECTION("inverted triangle") {
        Parfait::Point<double> a = {0, 0, 0};
        Parfait::Point<double> b = {0, 1, 0};
        Parfait::Point<double> c = {1, 0, 0};
        auto cost = MeshQuality::cellHilbertCost(inf::MeshInterface::TRI_3, {a, b, c});
        REQUIRE(cost > 1.0);
    }
}

TEST_CASE("Corner condition number") {
    using namespace inf;
    SECTION("A perfect hex corner") {
        Parfait::Point<double> e1 = {1, 0, 0};
        Parfait::Point<double> e2 = {0, 1, 0};
        Parfait::Point<double> e3 = {0, 0, 1};

        auto cond = MeshQuality::CornerConditionNumber::hex(e1, e2, e3);

        REQUIRE(cond == Approx(1.0));
    }

    SECTION("An isotropic tet") {
        Parfait::Point<double> e1 = {1, 0, 0};
        Parfait::Point<double> e2 = {0.5, sqrt(3.0) / 2.0, 0};
        Parfait::Point<double> e3 = {0.5, sqrt(3.0) / 6.0, sqrt(2.0) / sqrt(3.0)};

        auto cond = MeshQuality::CornerConditionNumber::isotropicTet(e1, e2, e3);
        REQUIRE(cond == Approx(1.0));
    }

    SECTION("A prism corner") {
        Parfait::Point<double> e1 = {1, 0, 0};
        Parfait::Point<double> e2 = {0.5, sqrt(3.0) / 2.0, 0};
        Parfait::Point<double> e3 = {0.0, 0.0, 1.0};

        auto cond = MeshQuality::CornerConditionNumber::prism(e1, e2, e3);
        REQUIRE(cond == Approx(1.0));
    }

    SECTION("pyramid bottom corner") {
        Parfait::Point<double> e1 = {0, 1, 0};
        Parfait::Point<double> e2 = {-1, 0, 0};
        Parfait::Point<double> e3 = {-sqrt(2.0) / 2.0, sqrt(2.0) / 2.0, 0.5};

        // not exact, but close enough
        auto cond = MeshQuality::CornerConditionNumber::pyramidBottomCorner(e1, e2, e3);
        REQUIRE(cond > 0.8);
    }

    SECTION("pyramid top corner") {
        Parfait::Point<double> e1 = {1, 1, -0.5};
        Parfait::Point<double> e2 = {-1, 1, -0.5};
        Parfait::Point<double> e3 = {-1, -1, -0.5};

        // not exact, but close enough
        auto cond = MeshQuality::CornerConditionNumber::pyramidTopCorner(e1, e2, e3);
        REQUIRE(cond > 0.8);
    }
}

TEST_CASE("Calculate the condition number of a hex") {
    using namespace inf;
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {1, 0, 0};
    Parfait::Point<double> p3 = {1, 1, 0};
    Parfait::Point<double> p4 = {0, 1, 0};
    Parfait::Point<double> p5 = {0, 0, 1};
    Parfait::Point<double> p6 = {1, 0, 1};
    Parfait::Point<double> p7 = {1, 1, 1};
    Parfait::Point<double> p8 = {0, 1, 1};

    auto cond = MeshQuality::ConditionNumber::hex(p1, p2, p3, p4, p5, p6, p7, p8);
    REQUIRE(cond == Approx(1.0));
}

TEST_CASE("Calculate the condition number of a prism") {
    using namespace inf;
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {1, 0, 0};
    Parfait::Point<double> p3 = {0.5, sqrt(3.0) / 2.0, 0};
    Parfait::Point<double> p4 = {0, 0, 1};
    Parfait::Point<double> p5 = {1, 0, 1};
    Parfait::Point<double> p6 = {0.5, sqrt(3.0) / 2.0, 1};

    auto cond = MeshQuality::ConditionNumber::prism(p1, p2, p3, p4, p5, p6);
    REQUIRE(cond > 0.8);
}
TEST_CASE("Calculate the condition number of a pyramid") {
    using namespace inf;
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {1, 0, 0};
    Parfait::Point<double> p3 = {1, 1, 0};
    Parfait::Point<double> p4 = {0, 1, 0};
    Parfait::Point<double> p5 = {.5, .5, .5};

    auto cond = MeshQuality::ConditionNumber::pyramid(p1, p2, p3, p4, p5);
    REQUIRE(cond > 0.8);
}

TEST_CASE("Condition number of a quad"){
    using namespace inf;
    Parfait::Point<double> p1 = {1, 0, 0};
    Parfait::Point<double> p2 = {0, 1, 0};

    auto c = MeshQuality::CornerConditionNumber::quad(p1, p2);
    REQUIRE(c == 1.0);
}

TEST_CASE("Cell flattness cost function on a perfect quad"){
    using namespace inf;
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {1, 0, 0};
    Parfait::Point<double> p3 = {1, 1, 0};
    Parfait::Point<double> p4 = {0, 1, 0};

    auto c = MeshQuality::Flattness::quad(p1, p2, p3, p4);
    REQUIRE(c == Approx(0.0).margin(1e-5));
}

TEST_CASE("Cell flattness cost function on a warped quad"){
    using namespace inf;
    Parfait::Point<double> p1 = {0, 0, 0};
    Parfait::Point<double> p2 = {1, 0, 0};
    Parfait::Point<double> p3 = {1, 1, 1};
    Parfait::Point<double> p4 = {0, 1, 0};

    auto c = MeshQuality::Flattness::quad(p1, p2, p3, p4);
    REQUIRE(c > 0);
    REQUIRE(c < 1.0);
}

TEST_CASE("Optimize a single tet", "[tet]"){
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh_data = inf::mock::oneTet();
    mesh_data.points[3] = {0,0,-1};
    auto mesh = std::make_shared<inf::TinfMesh>(mesh_data,mp.Rank());
    auto opt = inf::MeshOptimization(mp, mesh.get());
    opt.setThreshold(0.0);
    opt.finalize();

    auto cell_cost = inf::MeshQuality::cellHilbertCost(*mesh);
    for(int i = 0; i < 100; i++){
        opt.step();
    }
    cell_cost = inf::MeshQuality::cellHilbertCost(*mesh);
    REQUIRE(cell_cost.front() == Approx(0.0).margin(1e-5));
}
TEST_CASE("Optimize a single prism", "[prism]"){
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh_data = inf::mock::onePrism();
    Parfait::wiggle(mesh_data.points, 0.9);
    auto mesh = std::make_shared<inf::TinfMesh>(mesh_data, mp.Rank());
    auto opt = inf::MeshOptimization(mp, mesh.get());
    opt.setThreshold(0.0);
    opt.finalize();

    auto cell_cost = inf::MeshQuality::cellHilbertCost(*mesh);
    for(int i = 0; i < 100; i++){
        opt.step();
    }
    cell_cost = inf::MeshQuality::cellHilbertCost(*mesh);
    REQUIRE(cell_cost.front() == Approx(0.0).margin(1e-5));
}
TEST_CASE("Optimize a single pyramid", "[pyramid]"){
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh_data = inf::mock::onePyramid();
    mesh_data.points[4] = {0,0,-1};
    auto mesh = std::make_shared<inf::TinfMesh>(mesh_data, mp.Rank());
    auto opt = inf::MeshOptimization(mp, mesh.get());
    opt.setThreshold(0.0);
    opt.finalize();

    auto cell_cost = inf::MeshQuality::cellHilbertCost(*mesh);
    for(int i = 0; i < 100; i++){
        opt.step();
    }
    cell_cost = inf::MeshQuality::cellHilbertCost(*mesh);
    REQUIRE(cell_cost.front() < 0.012); // still haven't gotten pyramids to a perfect cost.
}
