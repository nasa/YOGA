#include <MessagePasser/MessagePasser.h>
#include <parfait/ParallelExtent.h>
#include <parfait/RecursiveBisection.h>
#include <parfait/ToString.h>
#include <RingAssertions.h>

TEST_CASE("Collapse and uniquely merge") {
    std::set<int> a = {1, 2, 3, 4};
    std::set<int> b = {1, 2, 3, 4, 8};

    std::map<int, int> a_old_to_new, b_old_to_new;
    std::tie(a_old_to_new, b_old_to_new) = Parfait::collapseAndMap(a, b);
    REQUIRE(a_old_to_new.at(1) == 0);
    REQUIRE(a_old_to_new.at(2) == 1);
    REQUIRE(a_old_to_new.at(3) == 2);
    REQUIRE(a_old_to_new.at(4) == 3);

    REQUIRE(b_old_to_new.at(1) == 4);
    REQUIRE(b_old_to_new.at(2) == 5);
    REQUIRE(b_old_to_new.at(3) == 6);
    REQUIRE(b_old_to_new.at(4) == 7);
    REQUIRE(b_old_to_new.at(8) == 8);
}

TEST_CASE("RCB partitioning") {
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.Rank() == 0) {
        auto points = Parfait::generateRandomPoints(100);

        int num_partitions = 7;
        auto part_ids = Parfait::recursiveBisection(points, num_partitions, 1.0e-4);
        std::set<int> unique_part_ids(part_ids.begin(), part_ids.end());
        REQUIRE(unique_part_ids == std::set<int>{0, 1, 2, 3, 4, 5, 6});
        std::vector<double> colors(part_ids.size());
        std::vector<int> color_count(7);
        for (size_t i = 0; i < part_ids.size(); i++) {
            colors[i] = double(part_ids[i]);
            color_count[part_ids[i]]++;
        }
        //        for (auto count : color_count) printf("color count %d\n", count);
        //        Parfait::PointWriter::write("rcb-points-serial", points, colors);
    }
}

TEST_CASE("start with points only on one rank") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<Parfait::Point<double>> points;
    if (mp.Rank() == 1) {
        points = Parfait::generateRandomPoints(100);
    }

    auto part = Parfait::recursiveBisection(mp, points, mp.NumberOfProcesses(), 1.0e-4);

    std::vector<int> counts_per_rank(mp.NumberOfProcesses(), 0);
    for (int rank : part) {
        counts_per_rank[rank]++;
    }

    if (mp.Rank() == 1) {
        int target = part.size() / mp.NumberOfProcesses();
        for (int rank = 0; rank < mp.NumberOfProcesses(); rank++) {
            REQUIRE(counts_per_rank[rank] < target + 5);
            REQUIRE(counts_per_rank[rank] > target - 5);
        }
    }
}

TEST_CASE("partition when number of things is less than number of ranks") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<Parfait::Point<double>> points;
    if (mp.Rank() == 0) {
        points = Parfait::generateRandomPoints(1);
    }

    auto part = Parfait::recursiveBisection(mp, points, mp.NumberOfProcesses(), 1.0e-4);
    if (mp.Rank() == 0) {
        REQUIRE(0 == part.front());
    } else {
        REQUIRE(0 == part.size());
    }
}

TEST_CASE("RCB find the center in serial") {
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() == 1) {
        auto points = Parfait::linspace(Parfait::Point<double>{0, 0, 0}, Parfait::Point<double>{1, 1, 1}, 100);
        auto domain = Parfait::ExtentBuilder::build(points);
        int dimension = 0;
        auto center = domain.center()[dimension];
        auto c = Parfait::findCenter(domain, points, 0.55, center, dimension);
        REQUIRE(c == Approx(0.55).margin(0.5));
    }
}

TEST_CASE("don't die if zero points are sent in") {
    REQUIRE_NOTHROW(Parfait::recursiveBisection(std::vector<Parfait::Point<double>>{}, 2, 1.0e-4));
    auto part = Parfait::recursiveBisection(std::vector<Parfait::Point<double>>{}, 2, 1.0e-4);
    REQUIRE(part.size() == 0);
}

TEST_CASE("throw if asked for zero partitions") {
    std::vector<Parfait::Point<double>> points = {{0, 0, 0}};
    REQUIRE_THROWS(Parfait::recursiveBisection(points, 0, 1.0e-4));
}

TEST_CASE("RCB find center with weighted nodes in serial") {
    MessagePasser mp(MPI_COMM_SELF);
    auto points = Parfait::linspace(Parfait::Point<double>{0, 0, 0}, Parfait::Point<double>{1, 1, 1}, 100);
    std::vector<double> node_weights(points.size(), 1.0);
    //-----double weights in first half
    for (int i = 0; i < 50; i++) {
        node_weights[i] = 2.0;
    }
    auto domain = Parfait::ExtentBuilder::build(points);
    int dimension = 0;
    auto center = domain.center()[dimension];
    auto c = Parfait::findCenter(domain, points, node_weights, 1.0 / 3.0, center, dimension);
    REQUIRE(c == Approx(0.25).margin(0.02));
}

TEST_CASE("RCB find center with weighted nodes in parallel") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<Parfait::Point<double>> total_points;
    std::vector<double> total_weights;
    if (mp.Rank() == 0) {
        total_points = Parfait::linspace(Parfait::Point<double>{0, 0, 0}, Parfait::Point<double>{1, 1, 1}, 100);
        total_weights = std::vector<double>(total_points.size(), 1.0);
        //-----double weights in first half
        for (size_t i = 0; i < total_weights.size() / 2; i++) {
            total_weights[i] = 2.0;
        }
    }
    auto points = mp.Balance(total_points, 0, mp.NumberOfProcesses());
    auto weights = mp.Balance(total_weights, 0, mp.NumberOfProcesses());

    auto domain = Parfait::ExtentBuilder::build(points);
    domain = Parfait::ParallelExtent::getBoundingBox(mp, domain);
    int dimension = 0;
    auto center = domain.center()[dimension];
    auto c = Parfait::findCenter(mp, domain, points, weights, 1.0 / 3.0, center, dimension);
    REQUIRE(c == Approx(0.25).margin(0.02));
}

TEST_CASE("RCB find the cutting ratio") {
    REQUIRE(Parfait::calcCutRatio(2) == Approx(0.5));
    REQUIRE(Parfait::calcCutRatio(3) == Approx(1.0 / 3.0));
    REQUIRE(Parfait::calcCutRatio(101) == Approx(50.0 / 101.0));
}

TEST_CASE("find center in parallel") {
    MessagePasser mp(MPI_COMM_WORLD);
    double dx = 1.0 / mp.NumberOfProcesses();
    double lo_x = dx * mp.Rank();
    Parfait::Extent<double> my_point_box({lo_x, 0, 0}, {lo_x + dx, 1, 1});
    auto points = Parfait::generateRandomPoints(10000, my_point_box);
    auto domain = Parfait::ExtentBuilder::build(points);
    domain = Parfait::ParallelExtent::getBoundingBox(mp, domain);
    int dimension = 0;
    auto center = domain.center()[dimension];
    auto c = Parfait::findCenter(mp, domain, points, 0.55, center, dimension);
    REQUIRE(c == Approx(0.55).margin(0.02));
}

TEST_CASE("Is within tolerance") {
    double target_ratio = 3.0 / 7.0;
    auto good = Parfait::isWithinTol(target_ratio, 100, 100, 0.01);
    REQUIRE(not good);
}

TEST_CASE("understanding bit shifting for uniqueness") {
    int leaf_id = 0;
    int left = leaf_id << 1;
    int right = (leaf_id << 1) + 1;
    REQUIRE(left == 0);
    REQUIRE(right == 1);
}

TEST_CASE("parallel RCB partitioning, target partitions match NumberOfProcesses") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto points = Parfait::generateRandomPoints(100000);

    int num_partitions = mp.NumberOfProcesses();
    auto part_ids = Parfait::recursiveBisection(mp, points, num_partitions, 1.0e-4);
    std::vector<double> colors(part_ids.size());
    std::vector<int> color_count(num_partitions);
    for (size_t i = 0; i < part_ids.size(); i++) {
        colors[i] = double(part_ids[i]);
        color_count[part_ids[i]]++;
    }
    Parfait::PointWriter::write(mp, "rcb-points-parallel", points, colors);
}

TEST_CASE("parallel 4D RCB partitioning, target partitions match NumberOfProcesses") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto points = Parfait::generateCartesianPoints(50, 50, 50);

    std::vector<Parfait::Point<double, 4>> points_4d(points.size());
    Parfait::Point<double> origin{0, 0, 0};
    for (size_t i = 0; i < points.size(); i++) {
        auto& p = points[i];
        double radius = Parfait::Point<double>::distance(origin, p);
        points_4d[i] = Parfait::Point<double, 4>(p[0], p[1], p[2], radius);
    }

    int num_partitions = mp.NumberOfProcesses() * 17;
    auto part_ids = Parfait::recursiveBisection(mp, points_4d, num_partitions, 1.0e-4);
    std::vector<double> colors(part_ids.size());
    std::vector<int> color_count(num_partitions);
    for (size_t i = 0; i < part_ids.size(); i++) {
        colors[i] = double(part_ids[i]);
        color_count[part_ids[i]]++;
    }
    Parfait::PointWriter::write(mp, "rcb-4d-points-parallel", points, colors);
}

TEST_CASE("parallel 6D RCB partitioning") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto points = Parfait::generateCartesianPoints(50, 50, 50);

    std::vector<Parfait::Point<double, 6>> points_6d(points.size());
    Parfait::Point<double> origin{0, 0, 0};
    for (size_t i = 0; i < points.size(); i++) {
        auto& p = points[i];
        auto r = p - origin;
        r.normalize();
        r *= 2.0;
        points_6d[i] = Parfait::Point<double, 6>(p[0], p[1], p[2], r[0], r[1], r[2]);
    }

    int num_partitions = mp.NumberOfProcesses() * 17;
    auto part_ids = Parfait::recursiveBisection(mp, points_6d, num_partitions, 1.0e-4);
    std::vector<double> colors(part_ids.size());
    std::vector<int> color_count(num_partitions);
    for (size_t i = 0; i < part_ids.size(); i++) {
        colors[i] = double(part_ids[i]);
        color_count[part_ids[i]]++;
    }
    Parfait::PointWriter::write(mp, "rcb-6d-points-parallel", points, colors);
}

TEST_CASE("parallel RCB partitioning, more parts than ranks") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto points = Parfait::generateRandomPoints(1000);

    int num_partitions = mp.NumberOfProcesses() + 1;
    auto part_ids = Parfait::recursiveBisection(mp, points, num_partitions, 1.0e-4);
    std::vector<double> colors(part_ids.size());
    std::vector<int> color_count(num_partitions);
    for (size_t i = 0; i < part_ids.size(); i++) {
        colors[i] = double(part_ids[i]);
        color_count[part_ids[i]]++;
    }
}

TEST_CASE("partition points on cartesian planes") {
    MessagePasser mp(MPI_COMM_WORLD);
    int num_partition = 40;
    auto points = Parfait::generateCartesianPoints(5, 5, 5);
    auto part_ids = Parfait::recursiveBisection(mp, points, num_partition, 1.0e-4);
    std::vector<double> colors(part_ids.size());
    std::vector<int> color_count(num_partition);
    for (size_t i = 0; i < part_ids.size(); i++) {
        colors[i] = double(part_ids[i]);
        color_count[part_ids[i]]++;
    }
    for (auto c : color_count) {
        REQUIRE(c != 0);
    }
}

#if 0
TEST_CASE("parallel RCB partitioning, more ranks than parts") {
    MessagePasser mp(MPI_COMM_WORLD);
    if(mp.NumberOfProcesses() == 1) return;
    auto points = Parfait::generateRandomPoints(1000);

    int num_partitions = mp.NumberOfProcesses() - 1;
    auto part_ids = Parfait::recursiveBisection(mp, points, num_partitions);
    std::vector<double> colors(part_ids.size());
    std::vector<int> color_count(num_partitions);
    for (size_t i = 0; i < part_ids.size(); i++) {
        colors[i] = double(part_ids[i]);
        color_count[part_ids[i]]++;
    }
}
#endif
