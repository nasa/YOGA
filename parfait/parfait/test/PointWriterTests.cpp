#include <MessagePasser/MessagePasser.h>
#include <parfait/PointWriter.h>
#include <parfait/PointGenerator.h>
#include <RingAssertions.h>

TEST_CASE("Point writer tests in parallel") {
    MessagePasser mp(MPI_COMM_WORLD);
    auto lo = static_cast<double>(mp.Rank());
    auto hi = static_cast<double>(mp.Rank() + 1);
    auto points = Parfait::generateRandomPoints(100, {{lo, lo, lo}, {hi, hi, hi}});
    Parfait::PointWriter::write(mp, "random-points.", points);
}
