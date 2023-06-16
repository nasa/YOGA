#include <RingAssertions.h>
#include <t-infinity/StructuredMeshSyncer.h>

using namespace inf;

class MockValueWrapper : public StructuredMeshValues<long> {
  public:
    using Values = std::map<int, Vector3D<long>>;
    MockValueWrapper(Values& values_in) : values(values_in) {}

    long getValue(int block_id, int i, int j, int k, int var) const override { return values[block_id](i, j, k); }
    void setValue(int block_id, int i, int j, int k, int var, const long& p) override { values[block_id](i, j, k) = p; }
    int blockSize() const override { return 1; }

  private:
    Values& values;
};

TEST_CASE("Can sync nodes", "[mpi]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::map<int, std::array<long, 2>> ownership_ranges;
    std::map<int, int> block_to_rank;
    block_to_rank[0] = 0;
    if (mp.NumberOfProcesses() == 1) {
        ownership_ranges[0] = {0, 11};
    } else {
        ownership_ranges[0] = {0, 7};
        ownership_ranges[1] = {8, 11};
        block_to_rank[1] = mp.NumberOfProcesses() - 1;
    }
    StructuredMeshGlobalIdOwner node_owners(ownership_ranges, block_to_rank);

    StructuredBlockGlobalIds global_node_ids;
    MockValueWrapper::Values values;
    if (mp.Rank() == 0) {
        long index = 0;
        global_node_ids[0] = Vector3D<long>({2, 2, 2});
        values[0] = Vector3D<long>({2, 2, 2});
        loopMDRange({0, 0, 0}, values[0].dimensions(), [&](int i, int j, int k) {
            values[0](i, j, k) = index;
            global_node_ids[0](i, j, k) = index++;
        });
    }
    if (mp.Rank() == mp.NumberOfProcesses() - 1) {
        long index = 8;
        global_node_ids[1] = Vector3D<long>({2, 2, 2});
        values[1] = Vector3D<long>({2, 2, 2});
        loopMDRange({0, 0, 0}, values[1].dimensions(), [&](int i, int j, int k) {
            values[1](i, j, k) = i == 0 ? -1 : index;
            global_node_ids[1](i, j, k) = i == 0 ? i : index++;
        });
    }
    auto syncer = StructuredMeshSyncer(mp, {global_node_ids, node_owners});
    MockValueWrapper value_wrapper(values);
    syncer.sync(value_wrapper);
    if (mp.Rank() == 0) {
        loopMDRange({0, 0, 0}, global_node_ids[0].dimensions(), [&](int i, int j, int k) {
            REQUIRE(values[0](i, j, k) == global_node_ids[0](i, j, k));
        });
    }
    if (mp.Rank() == mp.NumberOfProcesses() - 1) {
        loopMDRange({0, 0, 0}, global_node_ids[1].dimensions(), [&](int i, int j, int k) {
            REQUIRE(values[1](i, j, k) == global_node_ids[1](i, j, k));
        });
    }
}
