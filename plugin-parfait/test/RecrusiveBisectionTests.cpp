#include <MessagePasser/MessagePasser.h>
#include <parfait/KMeans.h>
#include <parfait/ParallelExtent.h>
#include <RingAssertions.h>

namespace RecursiveBisection {
template <typename T>
int largestDimension(const Parfait::Extent<T>& e) {
    std::array<T, 3> dist;
    for (int i = 0; i < 3; i++) dist[i] = e.hi[i] - e.lo[i];
    return std::distance(dist.begin(), std::max_element(dist.begin(), dist.begin() + 3));
}

}

TEST_CASE("largest dimension of extent") {
    int biggest_domain = RecursiveBisection::largestDimension(Parfait::Extent<double>{{0, 0, 0}, {2, 1, 1}});
    REQUIRE(biggest_domain == 0);
}

TEST_CASE("Parallel binary search") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::vector<Parfait::Point<double>> points_left(10, {0, 0, 0});
}

TEST_CASE("recursive bisection exists") {
    //    auto mp = MessagePasser(MPI_COMM_WORLD);
    //    auto points = Parfait::generateRandomPoints(1000);
    //    auto total_point_count = mp.ParallelSum(points.size());
    //    auto domain = Parfait::ExtentBuilder::build(points);
    //    auto total_domain = Parfait::ParallelExtent::getBoundingBox(mp, domain);
}
