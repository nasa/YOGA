#include <RingAssertions.h>
#include <parfait/StringTools.h>
#include <t-infinity/Snap.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <thread>
#include <chrono>
#include "../../unit_tests/MockMeshes.h"
#include <t-infinity/FieldLoader.h>
#include <t-infinity/PluginLocator.h>

using namespace inf;

TEST_CASE("Can read snap file via FieldLoader") {
    MessagePasser mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;

    auto mesh = mock::getSingleTetMesh();

    auto hash = Parfait::StringTools::randomLetters(6);
    std::vector<double> velocity(3 * mesh->nodeCount());
    for (int i = 0; i < mesh->nodeCount(); i++) {
        velocity[3 * i + 0] = i;
        velocity[3 * i + 1] = 10 * i;
        velocity[3 * i + 2] = 100 * i;
    }

    auto velocity_field =
        std::make_shared<inf::VectorFieldAdapter>("velocity", inf::FieldAttributes::Node(), 3, velocity);

    inf::Snap snap(mp.getCommunicator());
    snap.addMeshTopologies(*mesh);
    snap.add(velocity_field);

    snap.writeFile(hash);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto field_loader = getFieldLoader(INFINITY_UTILS_DIR, "libSnapReader");
    field_loader->load(hash, mp.getCommunicator(), *mesh);
    auto fields = field_loader->availableFields();
    REQUIRE(1 == fields.size());
    REQUIRE("velocity" == fields.front());
}
