#include <DonorPackager.h>
#include <parfait/Point.h>
#include <RingAssertions.h>

class MockMeshInteractor {
  public:
    double distanceAtPointInCell(int cell_id, const Parfait::Point<double>& p) const {
        Parfait::Point<double> nearest_surface_point{0, 0, 0};
        return Parfait::Point<double>::distance(p, nearest_surface_point);
    }
    int componentIdForCell(int cell_id) const { return 3; }
    int cellOwner(int cell_id) const { return 12; }
};

using namespace YOGA;

TEST_CASE("given query points and donor lists, package donor info") {
    std::vector<QueryPoint> query_points{{{0, 0, 0}, 7, 0}, {{1, 1, 1}, 8, 0}, {{2, 2, 2}, 9, 0}};
    std::vector<std::vector<int>> donor_ids{{0, 1}, {1}, {}};

    MockMeshInteractor mesh_interactor;

    auto donor_info = DonorPackager::package(query_points, donor_ids, mesh_interactor, 12);

    REQUIRE(query_points.size() == donor_info.size());
    REQUIRE(2 == donor_info[0].size());
    REQUIRE(0 == donor_info[0][0].cell_id);
    REQUIRE(3 == donor_info[0][0].component_id);
    REQUIRE(12 == donor_info[0][0].owner);
    REQUIRE(0.0 == donor_info[0][0].distance);

    REQUIRE(1 == donor_info[0][1].cell_id);
    REQUIRE(0.0 == donor_info[0][1].distance);

    REQUIRE(1 == donor_info[1].size());
    REQUIRE(std::sqrt(3.0) == Approx(donor_info[1][0].distance));

    REQUIRE(0 == donor_info[2].size());
}
