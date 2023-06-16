#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <parfait/PointwiseSources.h>

TEST_CASE("Given points, spacing, and decay values, write pcd file") {
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() > 1) return;

    std::vector<Parfait::Point<double>> points;
    points.push_back({0, 0, 0});
    points.push_back({1, 0, 0});
    points.push_back({0, 1, 0});
    points.push_back({0, 0, 1});

    std::vector<double> spacing(4, 1e-4);
    std::vector<double> decay(4, 0.5);

    std::string filename = "test.pcd";
    Parfait::PointwiseSources pointwise_sources(points, spacing, decay);
    pointwise_sources.write(filename);

    Parfait::FileTools::waitForFile(filename, 5);

    Parfait::PointwiseSources sources2(filename);

    REQUIRE(points == sources2.points);
    REQUIRE(spacing == sources2.spacing);
    REQUIRE(decay == sources2.decay);
}
