#include <RingAssertions.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/SMesh.h>
#include <t-infinity/StructuredMeshShuffle.h>
#include <t-infinity/StructuredMeshGlobalIds.h>
#include <t-infinity/StructuredMeshSyncer.h>

using namespace inf;

class ShuffleTestWrapper : public StructuredMeshValues<int> {
  public:
    explicit ShuffleTestWrapper(const MessagePasser& mp) : mp(mp) {}
    int getValue(int block_id, int i, int j, int k, int var) const override { return block_id; }
    void setValue(int block_id, int i, int j, int k, int var, const int& p) override {
        REQUIRE(block_id == p);
        REQUIRE(block_id == mp.Rank());
    }
    int blockSize() const override { return 1; }
    MessagePasser mp;
};

TEST_CASE("Can shuffle a structured mesh", "[mpi]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;

    auto mesh = std::make_shared<inf::SMesh>();

    if (mp.Rank() == 0) {
        std::array<int, 3> dimensions = {2, 2, 2};
        mesh->add(std::make_shared<CartesianBlock>(Parfait::Point<double>{0, 0, 0}, Parfait::Point<double>{1, 1, 1}, dimensions, 0));
        mesh->add(std::make_shared<CartesianBlock>(Parfait::Point<double>{1, 0, 0}, Parfait::Point<double>{2, 1, 1}, dimensions, 1));
    }

    auto expected_block_ids = mp.Rank() == 0 ? std::vector<int>{0, 1} : std::vector<int>{};
    REQUIRE(expected_block_ids == mesh->blockIds());

    std::map<int, int> block_to_rank;
    block_to_rank[0] = 0;
    block_to_rank[1] = 1;

    auto shuffled_mesh = shuffleStructuredMesh(mp, *mesh, block_to_rank);

    SECTION("shuffle mesh blocks") {
        auto actual_block_ids = shuffled_mesh->blockIds();
        REQUIRE(std::vector<int>{mp.Rank()} == actual_block_ids);
    }
    SECTION("build cell global ids") {
        auto original_global_ids = assignStructuredMeshGlobalCellIds(mp, *mesh);
        auto shuffled_global_ids = assignStructuredMeshGlobalCellIds(mp, *shuffled_mesh);
        auto sync_pattern = buildStructuredMeshtoMeshSyncPattern(mp, original_global_ids, shuffled_global_ids);
        auto source = StructuredMeshSyncer::buildGlobalToBlockIJK(mp, original_global_ids);
        auto target = StructuredMeshSyncer::buildGlobalToBlockIJK(mp, shuffled_global_ids);
        auto values = ShuffleTestWrapper(mp);
        auto wrapper = ValueWrapper<int>(values, source, target);
        Parfait::syncStridedField<int>(mp, wrapper, sync_pattern);
    }
}
TEST_CASE("Can shuffle a structured mesh global ids", "[mpi]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    StructuredBlockGlobalIds ids;
    if (mp.Rank() == 0) {
        long gid = 0;
        ids[0] = Vector3D<long>({2, 2, 2});
        loopMDRange({0, 0, 0}, ids[0].dimensions(), [&](int i, int j, int k) { ids[0](i, j, k) = gid++; });
        ids[10] = Vector3D<long>({3, 2, 2});
        loopMDRange({0, 0, 0}, ids[10].dimensions(), [&](int i, int j, int k) { ids[10](i, j, k) = gid++; });
    }
    std::map<int, int> block_to_rank = {{0, 0}, {10, 0}};
    std::map<int, std::array<long, 2>> ownership_ranges = {{0, {0, 7}}, {10, {8, 20}}};
    StructuredMeshGlobalIdOwner owners(ownership_ranges, block_to_rank);
    StructuredMeshGlobalIds global_ids{ids, owners};

    std::map<int, int> shuffled_block_to_rank = {{0, mp.NumberOfProcesses() - 1}, {10, 0}};
    auto shuffled_globals = shuffleStructuredMeshGlobalIds(mp, global_ids, shuffled_block_to_rank);
    for (const auto& p : shuffled_block_to_rank) {
        int block_id = p.first;
        if (mp.Rank() == p.second) {
            REQUIRE(1 == shuffled_globals.ids.count(block_id));
            loopMDRange({0, 0, 0}, shuffled_globals.ids.at(block_id).dimensions(), [&](int i, int j, int k) {
                long gid = shuffled_globals.ids.at(block_id)(i, j, k);
                REQUIRE(block_id == shuffled_globals.owner.getOwningBlock(gid));
                REQUIRE(mp.Rank() == shuffled_globals.owner.getOwningRank(gid));
            });
        }
    }
}
