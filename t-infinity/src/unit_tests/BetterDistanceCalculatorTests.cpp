#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <parfait/Facet.h>
#include <parfait/PointGenerator.h>
#include <t-infinity/BetterDistanceCalculator.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Shortcuts.h>

TEST_CASE("Better distance calculator can bring metadata") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    double r = mp.Rank();
    Parfait::Facet f = {{0, 0, r}, {1, 0, r}, {1, 1, r}};
    Parfait::Point<double> p = {.5, .5, r};

    std::vector<double> point_distance, search_cost;

    std::tie(point_distance, search_cost) = inf::calcBetterDistance(mp, {f}, {p});
    REQUIRE(point_distance.size() == 1);
    REQUIRE(search_cost.size() == 1);
    REQUIRE(point_distance[0] == Approx(0.0).margin(1.0e-10));
}

TEST_CASE("Better distance can communicate trivially copyable metadata") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    double r = mp.Rank();
    Parfait::Facet f = {{0, 0, r}, {1, 0, r}, {1, 1, r}};
    Parfait::Point<double> p = {.5, .5, r};
    using MetaData = int;
    std::vector<MetaData> meta_data(1);
    meta_data[0] = mp.Rank();

    std::vector<double> point_distance, search_cost;
    std::vector<MetaData> meta_data_out;

    std::tie(point_distance, search_cost, meta_data_out) =
        inf::calcBetterDistanceWithMetaData(mp, {f}, {p}, meta_data);
    REQUIRE(point_distance.size() == 1);
    REQUIRE(search_cost.size() == 1);
    REQUIRE(meta_data_out.size() == 1);
    REQUIRE(meta_data[0] == mp.Rank());
    REQUIRE(point_distance[0] == Approx(0.0).margin(1.0e-10));
}

TEST_CASE("Better distance works on 2D problems", "[distance2d]") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create2D(mp, 100, 100);
    auto points = inf::extractPoints(*mesh);

    auto circle_points =
        Parfait::generateOrderedPointsOnCircleInXYPlane(1000, {0.5, 0.5, 0.0}, 0.2);
    std::vector<Parfait::LineSegment> segments;
    for (int i = 0; i < int(circle_points.size()); i++) {
        int j = (i + 1) % int(circle_points.size());
        segments.push_back({circle_points[i], circle_points[j]});
    }
    auto distance = inf::calcBetterDistance2D(mp, segments, points);

    inf::shortcut::visualize_at_nodes("circle-distance-2d", mp, mesh, {distance}, {"distance"});
}