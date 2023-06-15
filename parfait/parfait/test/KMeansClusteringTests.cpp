#include <RingAssertions.h>
#include <parfait/KMeans.h>
#include <parfait/PointGenerator.h>

TEST_CASE("KMeans point cloud clustering in parallel") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::vector<Parfait::Point<double>> points;
    if (mp.Rank() % 2 == 0) {
        auto points_lower = Parfait::generateRandomPoints(1e2, {{0, 0, 0}, {1, 1, 1}});
        points.insert(points.end(), points_lower.begin(), points_lower.end());
    } else {
        auto points_upper = Parfait::generateRandomPoints(1e2, {{.75, .75, .75}, {1.75, 1.75, 1.75}});
        points.insert(points.end(), points_upper.begin(), points_upper.end());
    }

    int num_clusters = 2;
    auto cluster = Parfait::KMeansClustering::apply(mp, points, num_clusters);

    std::vector<double> cluster_double(cluster.size());
    for (size_t i = 0; i < cluster.size(); i++) cluster_double[i] = static_cast<double>(cluster[i]);
    auto extents = Parfait::createExtentsForCategories(points, cluster);
    std::vector<double> extent_id(extents.size());
    std::iota(extent_id.begin(), extent_id.end(), 0);

    //    Parfait::PointWriter::write(mp, "k-means-random-points", points, cluster_double);
    //    Parfait::ExtentWriter::write(mp, "random-extents", extents, extent_id);
}

TEST_CASE("KMeans point cloud clustering") {
    auto points_lower = Parfait::generateRandomPoints(1e2, {{0, 0, 0}, {1, 1, 1}});
    auto points_upper = Parfait::generateRandomPoints(1e2, {{.75, .75, .75}, {1.75, 1.75, 1.75}});
    std::vector<Parfait::Point<double>> points;
    points.insert(points.end(), points_lower.begin(), points_lower.end());
    points.insert(points.end(), points_upper.begin(), points_upper.end());

    int num_clusters = 15;
    auto cluster = Parfait::KMeansClustering::apply(points, num_clusters);
    std::vector<double> cluster_double(cluster.size());
    for (size_t i = 0; i < cluster.size(); i++) cluster_double[i] = static_cast<double>(cluster[i]);
    auto extents = Parfait::createExtentsForCategories(points, cluster);
    std::vector<double> extent_id(extents.size());
    std::iota(extent_id.begin(), extent_id.end(), 0);

    //    Parfait::PointWriter::write("random-points", points, cluster_double);
    //    Parfait::ExtentWriter::write("random-extents", extents, extent_id);
}
