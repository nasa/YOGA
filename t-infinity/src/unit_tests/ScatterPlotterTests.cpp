#include <RingAssertions.h>
#include "../utilities/ScatterPlotter.h"

TEST_CASE("given point cloud in parallel make a single plot file") {
    MessagePasser mp(MPI_COMM_WORLD);

    auto get_n_pts = []() { return 1; };
    auto get_xyz = [&](int id, double* xyz) { xyz[0] = xyz[1] = xyz[2] = mp.Rank(); };
    auto get_gid = [&](int id, long* gid) { *gid = int(mp.Rank()); };
    auto get_scalar = [&](int id, double* gid) { *gid = double(mp.Rank()); };

    auto collapsed_pts = ScatterPlotter::collapse(mp, get_n_pts, get_xyz);
    auto collapsed_gids = ScatterPlotter::collapse<long>(mp, get_n_pts, get_gid);

    REQUIRE(int(collapsed_pts.size()) == mp.NumberOfProcesses());
    REQUIRE(int(collapsed_gids.size()) == mp.NumberOfProcesses());

    ScatterPlotter::plot("dog.csv", mp, get_n_pts, get_xyz, get_gid, get_scalar);
}
