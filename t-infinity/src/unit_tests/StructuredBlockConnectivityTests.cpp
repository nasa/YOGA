#include <RingAssertions.h>

#include <t-infinity/CartMesh.h>
#include <t-infinity/MDVector.h>
#include <t-infinity/SMesh.h>
#include <parfait/MotionMatrix.h>
#include <parfait/Throw.h>
#include <parfait/StringTools.h>
#include <parfait/FileTools.h>
#include <t-infinity/StructuredBlockConnectivity.h>
#include <t-infinity/StructuredMeshConnectivity.h>

using namespace inf;
using Point = Parfait::Point<double>;

TEST_CASE("Structured block to block connectivity") {
    auto block1 = CartesianBlock({0, 0, 0}, {1, 1, 1}, {2, 2, 2}, 0);
    SECTION("IMIN") {
        auto face_mapping = getBlockFaces(block1, IMIN);
        REQUIRE(0 == face_mapping.block_id);
        REQUIRE(IMIN == face_mapping.side);
        REQUIRE(face_mapping.corners[0].approxEqual({0, 0, 0}));
        REQUIRE(face_mapping.corners[1].approxEqual({0, 1, 0}));
        REQUIRE(face_mapping.corners[2].approxEqual({0, 1, 1}));
        REQUIRE(face_mapping.corners[3].approxEqual({0, 0, 1}));
    }
    SECTION("IMAX") {
        auto face_mapping = getBlockFaces(block1, IMAX);
        REQUIRE(0 == face_mapping.block_id);
        REQUIRE(IMAX == face_mapping.side);
        REQUIRE(face_mapping.corners[0].approxEqual({1, 0, 0}));
        REQUIRE(face_mapping.corners[1].approxEqual({1, 1, 0}));
        REQUIRE(face_mapping.corners[2].approxEqual({1, 1, 1}));
        REQUIRE(face_mapping.corners[3].approxEqual({1, 0, 1}));
    }
    SECTION("JMIN") {
        auto face_mapping = getBlockFaces(block1, JMIN);
        REQUIRE(0 == face_mapping.block_id);
        REQUIRE(JMIN == face_mapping.side);
        REQUIRE(face_mapping.corners[0].approxEqual({0, 0, 0}));
        REQUIRE(face_mapping.corners[1].approxEqual({1, 0, 0}));
        REQUIRE(face_mapping.corners[2].approxEqual({1, 0, 1}));
        REQUIRE(face_mapping.corners[3].approxEqual({0, 0, 1}));
    }
    SECTION("JMAX") {
        auto face_mapping = getBlockFaces(block1, JMAX);
        REQUIRE(0 == face_mapping.block_id);
        REQUIRE(JMAX == face_mapping.side);
        REQUIRE(face_mapping.corners[0].approxEqual({0, 1, 0}));
        REQUIRE(face_mapping.corners[1].approxEqual({1, 1, 0}));
        REQUIRE(face_mapping.corners[2].approxEqual({1, 1, 1}));
        REQUIRE(face_mapping.corners[3].approxEqual({0, 1, 1}));
    }
    SECTION("KMIN") {
        auto face_mapping = getBlockFaces(block1, KMIN);
        REQUIRE(0 == face_mapping.block_id);
        REQUIRE(KMIN == face_mapping.side);
        REQUIRE(face_mapping.corners[0].approxEqual({0, 0, 0}));
        REQUIRE(face_mapping.corners[1].approxEqual({1, 0, 0}));
        REQUIRE(face_mapping.corners[2].approxEqual({1, 1, 0}));
        REQUIRE(face_mapping.corners[3].approxEqual({0, 1, 0}));
    }
    SECTION("KMAX") {
        auto face_mapping = getBlockFaces(block1, KMAX);
        REQUIRE(0 == face_mapping.block_id);
        REQUIRE(KMAX == face_mapping.side);
        REQUIRE(face_mapping.corners[0].approxEqual({0, 0, 1}));
        REQUIRE(face_mapping.corners[1].approxEqual({1, 0, 1}));
        REQUIRE(face_mapping.corners[2].approxEqual({1, 1, 1}));
        REQUIRE(face_mapping.corners[3].approxEqual({0, 1, 1}));
    }
}

TEST_CASE("check block to block face corners match") {
    BlockFaceCorners face1 = {Point(0, 0, 0), Point(0, 1, 0), Point(0, 1, 1), Point(0, 0, 1)};
    BlockFaceCorners face2 = {Point(0, 0, 0), Point(0, 1, 1), Point(0, 1, 0), Point(0, 0, 1)};
    BlockFaceCorners face3 = {Point(0, 0, 0), Point(0, 1, 1), Point(0, 1, 0), Point(1, 1, 1)};
    REQUIRE(blockFacesMatch(face1, face2));
    REQUIRE_FALSE(blockFacesMatch(face1, face3));
}

TEST_CASE("Map tangent face axis") {
    for (auto side : {IMIN, JMIN, KMIN, IMAX, JMAX, KMAX}) {
        INFO("Side: " << side);
        auto checkSide = [&](int corner1, int corner2, BlockAxis expected_axis) {
            auto actual = getTangentFaceMapping(corner1, corner2, side);
            REQUIRE(expected_axis == actual.axis);
            REQUIRE(SameDirection == actual.direction);

            auto reversed = getTangentFaceMapping(corner2, corner1, side);
            REQUIRE(expected_axis == reversed.axis);
            REQUIRE(ReversedDirection == reversed.direction);
        };

        checkSide(0, 1, TangentAxis::sideToFirst(side));
        checkSide(3, 2, TangentAxis::sideToFirst(side));
        checkSide(0, 3, TangentAxis::sideToSecond(side));
        checkSide(1, 2, TangentAxis::sideToSecond(side));
    }
}

TEST_CASE("get face axis mapping") {
    using Point = Point;
    SECTION("non-degenerate, aligned") {
        BlockFace host{};
        host.block_id = 0;
        host.corners = {Point(0, 0, 0), Point(0, 1, 0), Point(0, 1, 1), Point(0, 0, 1)};
        host.side = IMIN;
        BlockFace neighbor{};
        neighbor.block_id = 1;
        neighbor.corners = host.corners;
        neighbor.side = IMAX;
        auto face_mapping = getFaceAxisMapping(host, neighbor);
        for (auto axis : {I, J, K}) {
            REQUIRE(axis == face_mapping[axis].axis);
            REQUIRE(SameDirection == face_mapping[axis].direction);
        }
    }
    SECTION("degenerate, aligned") {
        BlockFace host{};
        host.block_id = 0;
        host.corners = {Point(0, 0, 0), Point(0, 0.5, 0), Point(0, 0, 0.5), Point(0, 0, 0)};
        host.side = IMIN;
        BlockFace neighbor{};
        neighbor.block_id = 1;
        neighbor.corners = host.corners;
        neighbor.side = IMAX;
        auto face_mapping = getFaceAxisMapping(host, neighbor);
        for (auto axis : {I, J, K}) {
            REQUIRE(axis == face_mapping[axis].axis);
            REQUIRE(SameDirection == face_mapping[axis].direction);
        }
    }
}

TEST_CASE("map block ordinals") {
    SECTION("IMIN-IMAX") {
        BlockFace face1{}, face2{};
        face1.corners = {Point(0, 0, 0), Point(0, 1, 0), Point(0, 1, 1), Point(0, 0, 1)};
        face1.side = IMAX;
        face1.block_id = 0;

        face2.corners = {Point(0, 0, 0), Point(0, 1, 0), Point(0, 1, 1), Point(0, 0, 1)};
        face2.side = IMIN;
        face2.block_id = 1;

        auto connectivity = getFaceConnectivity(face1, face2);
        REQUIRE(connectivity.face_mapping[0].axis == BlockAxis::I);
        REQUIRE(connectivity.face_mapping[0].direction == AxisDirection::SameDirection);

        REQUIRE(connectivity.face_mapping[1].axis == BlockAxis::J);
        REQUIRE(connectivity.face_mapping[1].direction == AxisDirection::SameDirection);

        REQUIRE(connectivity.face_mapping[2].axis == BlockAxis::K);
        REQUIRE(connectivity.face_mapping[2].direction == AxisDirection::SameDirection);

        REQUIRE(1 == connectivity.neighbor_block_id);
        REQUIRE(IMIN == connectivity.neighbor_side);
    }
}

class UnitBlock : public inf::StructuredBlock {
  public:
    explicit UnitBlock(int block_id) : block_id(block_id), node_dimensions({2, 2, 2}), xyz(node_dimensions) {
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    xyz(i, j, k) = {double(i), double(j), double(k)};
                }
            }
        }
    }
    void move(Parfait::MotionMatrix& m) {
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    m.movePoint(xyz(i, j, k).data());
                }
            }
        }
    }
    std::array<int, 3> nodeDimensions() const override { return node_dimensions; }
    Point point(int i, int j, int k) const override { return xyz(i, j, k); }
    void setPoint(int i, int j, int k, const Point& p) override { xyz(i, j, k) = p; }
    int blockId() const override { return block_id; }

    int block_id;
    std::array<int, 3> node_dimensions;
    Vector3D<Point> xyz;
};

std::array<double, 3> getTranslation(BlockSide side) {
    switch (side) {
        case IMIN:
            return {-1, 0, 0};
        case JMIN:
            return {0, -1, 0};
        case KMIN:
            return {0, 0, -1};
        case IMAX:
            return {1, 0, 0};
        case JMAX:
            return {0, 1, 0};
        case KMAX:
            return {0, 0, 1};
        default:
            PARFAIT_THROW("Unknown side: " + std::to_string(side));
    }
}

TEST_CASE("map all face-to-face combinations") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto block1 = std::make_shared<UnitBlock>(0);
    for (auto side : AllBlockSides) {
        INFO("side: " << side);

        auto mesh = SMesh();

        if (mp.Rank() == 0) mesh.add(block1);

        if (mp.Rank() == mp.NumberOfProcesses() - 1) {
            auto m = Parfait::MotionMatrix(getTranslation(side).data());
            auto block2 = std::make_shared<UnitBlock>(1);
            block2->move(m);
            mesh.add(block2);
        }

        auto mesh_connectivity = buildMeshBlockConnectivity(mp, mesh);

        if (mp.Rank() == 0) {
            const auto& connectivity = mesh_connectivity.at(0).at(side);
            REQUIRE(connectivity.face_mapping[0].axis == BlockAxis::I);
            REQUIRE(connectivity.face_mapping[0].direction == AxisDirection::SameDirection);

            REQUIRE(connectivity.face_mapping[1].axis == BlockAxis::J);
            REQUIRE(connectivity.face_mapping[1].direction == AxisDirection::SameDirection);

            REQUIRE(connectivity.face_mapping[2].axis == BlockAxis::K);
            REQUIRE(connectivity.face_mapping[2].direction == AxisDirection::SameDirection);

            REQUIRE(1 == connectivity.neighbor_block_id);
            REQUIRE(getOppositeSide(side) == connectivity.neighbor_side);
        }
    }
}

TEST_CASE("Ensure that unconnected blocks are still initialized in MeshConnectivity") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto block1 = std::make_shared<UnitBlock>(0);
    auto mesh = SMesh();
    mesh.add(block1);

    int block_id = 1;
    for (auto side : AllBlockSides) {
        auto m = Parfait::MotionMatrix(getTranslation(side).data());
        auto block2 = std::make_shared<UnitBlock>(block_id++);
        block2->move(m);
        block2->move(m);
        mesh.add(block2);
    }
    auto mesh_connectivity = buildMeshBlockConnectivity(mp, mesh);
    REQUIRE(7 == mesh_connectivity.size());
    for (const auto& block : mesh_connectivity) REQUIRE(block.second.empty());
}

TEST_CASE("Can build MeshConnectivity from LAURA-style boundary conditions") {
    SECTION("aligned") {
        auto inf_connectivity = buildBlockConnectivityFromLAURABoundaryConditions({3, 1201111, 3, 3, 3, 3});
        REQUIRE(1 == inf_connectivity.size());
        const auto& actual = inf_connectivity.at(IMAX);
        REQUIRE(200 == actual.neighbor_block_id);
        REQUIRE(IMIN == actual.neighbor_side);
        REQUIRE(I == actual.face_mapping[I].axis);
        REQUIRE(inf::SameDirection == actual.face_mapping[I].direction);
        REQUIRE(J == actual.face_mapping[J].axis);
        REQUIRE(inf::SameDirection == actual.face_mapping[J].direction);
        REQUIRE(K == actual.face_mapping[K].axis);
        REQUIRE(inf::SameDirection == actual.face_mapping[K].direction);
    }
    SECTION("I - > J") {
        auto inf_connectivity = buildBlockConnectivityFromLAURABoundaryConditions({1201421, 3, 3, 3, 3, 3});
        REQUIRE(1 == inf_connectivity.size());
        const auto& actual = inf_connectivity.at(IMIN);
        REQUIRE(200 == actual.neighbor_block_id);
        REQUIRE(JMAX == actual.neighbor_side);
        REQUIRE(J == actual.face_mapping[I].axis);
        REQUIRE(inf::SameDirection == actual.face_mapping[I].direction);
        REQUIRE(I == actual.face_mapping[J].axis);
        REQUIRE(inf::ReversedDirection == actual.face_mapping[J].direction);
        REQUIRE(K == actual.face_mapping[K].axis);
        REQUIRE(inf::SameDirection == actual.face_mapping[K].direction);
    }
}

TEST_CASE("Build structured block connectivity from LAURA boundary conditions") {
    SECTION("less than 1000 blocks") {
        std::string laura_bound_data = R"(      4     1002111           5           5           0           3
                                          1001211           1           5           5           0           3)";
        auto filename = Parfait::StringTools::randomLetters(5);
        Parfait::FileTools::writeStringToFile(filename, laura_bound_data);
        auto connectivity = inf::buildMeshConnectivityFromLAURABoundaryConditions(filename);
        REQUIRE(1 == connectivity[0][inf::IMAX].neighbor_block_id);
        REQUIRE(0 == connectivity[1][inf::IMIN].neighbor_block_id);
        REQUIRE(0 == remove(filename.c_str()));
    }
    SECTION("more than 1000 blocks") {
        std::string laura_bound_data = R"(      4     10002111           5           5           0           3
                                          10001211           1           5           5           0           3)";
        auto filename = Parfait::StringTools::randomLetters(5);
        Parfait::FileTools::writeStringToFile(filename, laura_bound_data);
        auto connectivity = inf::buildMeshConnectivityFromLAURABoundaryConditions(filename);
        REQUIRE(1 == connectivity[0][inf::IMAX].neighbor_block_id);
        REQUIRE(0 == connectivity[1][inf::IMIN].neighbor_block_id);
        REQUIRE(0 == remove(filename.c_str()));
    }
}