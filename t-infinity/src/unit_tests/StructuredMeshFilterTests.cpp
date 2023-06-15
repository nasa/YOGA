#include <RingAssertions.h>
#include <t-infinity/StructuredMeshFilter.h>

using namespace inf;

TEST_CASE("Can filter a StructuredMesh by block sides") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = std::make_shared<SMesh>();
    auto block = SMesh::createBlock({3, 4, 5}, 0);
    loopMDRange({0, 0, 0}, block->nodeDimensions(), [&](int i, int j, int k) {
        Parfait::Point<double> p = {double(i), double(j), double(k)};
        block->setPoint(i, j, k, p);
    });
    mesh->add(block);

    StructuredBlockSideSelector::BlockSidesToFilter sides_to_filter;
    sides_to_filter[0] = {IMIN, JMAX};
    StructuredBlockSideSelector selector(mp, sides_to_filter);
    StructuredMeshFilter filter(mesh, selector);

    SECTION("selector produces correct ranges") {
        auto ranges = selector.getRanges(*mesh);
        REQUIRE(2 == ranges.size());
        REQUIRE(0 == ranges.at(0).block_id);
        REQUIRE(std::array<int, 3>{0, 0, 0} == ranges.at(0).min);
        REQUIRE(std::array<int, 3>{1, 4, 5} == ranges.at(0).max);
        REQUIRE(0 == ranges.at(1).block_id);
        REQUIRE(std::array<int, 3>{0, 3, 0} == ranges.at(1).min);
        REQUIRE(std::array<int, 3>{3, 4, 5} == ranges.at(1).max);
    }

    SECTION("filtered mesh is correct") {
        auto filtered_mesh = filter.getMesh();
        REQUIRE(2 == filtered_mesh->blockIds().size());
        const auto& imin = filtered_mesh->getBlock(0);
        loopMDRange({0, 0, 0}, imin.nodeDimensions(), [&](int i, int j, int k) {
            auto previous = imin.previousNodeBlockIJK(i, j, k);
            REQUIRE(0 == previous.block_id);
            REQUIRE(i == previous.i);
            REQUIRE(j == previous.j);
            REQUIRE(k == previous.k);
        });

        const auto& jmax = filtered_mesh->getBlock(1);
        loopMDRange({0, 0, 0}, jmax.nodeDimensions(), [&](int i, int j, int k) {
            auto previous = jmax.previousNodeBlockIJK(i, j, k);
            REQUIRE(0 == previous.block_id);
            REQUIRE(i == previous.i);
            REQUIRE(j + 3 == previous.j);
            REQUIRE(k == previous.k);
        });
    }

    SECTION("filtered field is correct") {
        auto field = std::make_shared<SField>("original field", FieldAttributes::Node());
        auto f = SField::createBlock(block->nodeDimensions(), block->blockId());
        loopMDRange({0, 0, 0}, f->dimensions(), [&](int i, int j, int k) { f->setValue(i, j, k, double(i)); });
        field->add(f);
        auto filtered_field = filter.apply(field);
        REQUIRE(2 == filtered_field->blockIds().size());
        const auto& imin = filtered_field->getBlockField(0);
        loopMDRange(
            {0, 0, 0}, imin.dimensions(), [&](int i, int j, int k) { REQUIRE(double(i) == imin.value(i, j, k)); });
        const auto& jmax = filtered_field->getBlockField(1);
        loopMDRange(
            {0, 0, 0}, jmax.dimensions(), [&](int i, int j, int k) { REQUIRE(double(i) == jmax.value(i, j, k)); });
    }
}

TEST_CASE("Can extract a StructuredMesh submesh based on node ranges") {
    auto mesh = std::make_shared<SMesh>();
    auto block = SMesh::createBlock({3, 4, 5}, 12);
    loopMDRange({0, 0, 0}, block->nodeDimensions(), [&](int i, int j, int k) {
        Parfait::Point<double> p = {double(i), double(j), double(k)};
        block->setPoint(i, j, k, p);
    });
    mesh->add(block);
    StructuredSubMesh::NodeRanges sub_mesh_range;
    sub_mesh_range[1].block_id = block->block_id;
    sub_mesh_range[1].min = {0, 2, 3};
    sub_mesh_range[1].max = {3, 4, 4};

    sub_mesh_range[2].block_id = block->block_id;
    sub_mesh_range[2].min = {1, 0, 0};
    sub_mesh_range[2].max = {2, 4, 5};

    StructuredSubMesh submesh(mesh, sub_mesh_range);
    const auto& subblock1 = submesh.getBlock(1);
    REQUIRE(std::array<int, 3>{3, 2, 1} == subblock1.nodeDimensions());
    loopMDRange({0, 0, 0}, subblock1.nodeDimensions(), [&](int i, int j, int k) {
        auto expected = block->point(i, j + 2, k + 3);
        auto actual = subblock1.point(i, j, k);
        REQUIRE(expected.approxEqual(actual));
        auto previous = subblock1.previousNodeBlockIJK(i, j, k);
        REQUIRE(12 == previous.block_id);
        REQUIRE(i == previous.i);
        REQUIRE(j + 2 == previous.j);
        REQUIRE(k + 3 == previous.k);
    });

    const auto& subblock2 = submesh.getBlock(2);
    REQUIRE(std::array<int, 3>{1, 4, 5} == subblock2.nodeDimensions());
    loopMDRange({0, 0, 0}, subblock2.nodeDimensions(), [&](int i, int j, int k) {
        auto expected = block->point(i + 1, j, k);
        auto actual = subblock2.point(i, j, k);
        REQUIRE(expected.approxEqual(actual));
        auto previous = subblock2.previousNodeBlockIJK(i, j, k);
        REQUIRE(12 == previous.block_id);
        REQUIRE(i + 1 == previous.i);
        REQUIRE(j == previous.j);
        REQUIRE(k == previous.k);
    });
}