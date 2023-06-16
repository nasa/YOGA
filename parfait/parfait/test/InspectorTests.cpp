#include <RingAssertions.h>
#include <parfait/Inspector.h>
#include <MessagePasser/MessagePasser.h>

using namespace Parfait;

TEST_CASE("Measure load imbalance with inspector") {
    MessagePasser mp(MPI_COMM_WORLD);
    Inspector inspector(mp, 0);
    inspector.begin("task 1");
    inspector.end("task 1");

    inspector.begin("another task");
    inspector.end("another task");

    inspector.collect();

    std::vector<double> vec(mp.NumberOfProcesses());
    for (int i = 0; i < mp.NumberOfProcesses(); i++) {
        vec[i] = i * i;
    }

    inspector.addExternalField("user-defined", vec);

    auto csv_string = inspector.asCSV();
    printf("%s\n", csv_string.c_str());
}
