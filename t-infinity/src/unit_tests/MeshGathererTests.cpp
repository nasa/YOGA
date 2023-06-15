#include <vector>
#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <parfait/Point.h>
#include <t-infinity/MeshGatherer.h>

TEST_CASE("Gather types") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<inf::MeshInterface::CellType> my_types;
    if (mp.Rank() == 0) {
        my_types.push_back(inf::MeshInterface::TETRA_4);
    } else {
        my_types.push_back(inf::MeshInterface::TRI_3);
    }

    auto all_types = inf::gatherTypes(mp, my_types);
    if (mp.NumberOfProcesses() == 1) {
        REQUIRE(all_types.size() == 1);
    } else {
        REQUIRE(all_types.size() == 2);
    }
}
