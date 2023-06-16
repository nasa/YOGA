#include <MessagePasser/MessagePasser.h>
#include <iostream>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/Snap.h>
#include <Tracer.h>
#include <parfait/ToString.h>

int main(int argc, char* argv[]) {
    MessagePasser::Init();
    MessagePasser mp(MPI_COMM_WORLD);

    long number_of_items_per_rank = 5 * 50000;
    size_t max_chunk_in_mb = 100;
    if (argc >= 2) number_of_items_per_rank = std::stoi(argv[1]);
    if (argc >= 3) max_chunk_in_mb = std::stoi(argv[2]);
    if (mp.Rank() == 0) {
        printf("Simulating grid of size %s\n",
               Parfait::bigNumberToStringWithCommas(number_of_items_per_rank * mp.NumberOfProcesses()).c_str());
        printf("With chunk size %ldMB\n", max_chunk_in_mb);
    }
    std::vector<double> some_vector(number_of_items_per_rank);
    std::vector<long> gids(number_of_items_per_rank);
    std::vector<bool> do_own(number_of_items_per_rank, true);

    for (int i = 0; i < number_of_items_per_rank; i++) {
        long global_index = mp.Rank() * number_of_items_per_rank + i;
        some_vector[i] = global_index;
        gids[i] = global_index;
    }
    auto field = std::make_shared<inf::VectorFieldAdapter>(
        "some_field", inf::FieldAttributes::Unassigned(), 1, some_vector);

    {
        auto start = std::chrono::system_clock::now();
        inf::Snap snap(mp.getCommunicator());
        snap.setTopology(inf::FieldAttributes::Unassigned(), gids, do_own);
        snap.add(field);
        snap.setMaxChunkSizeInMB(max_chunk_in_mb);
        snap.writeFile("dog.snap");
        auto end = std::chrono::system_clock::now();
        if (mp.Rank() == 0)
            std::cout << "Snap writing took: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
                      << " seconds" << std::endl;
    }

    {
        auto start = std::chrono::system_clock::now();
        inf::Snap snap_read(mp.getCommunicator());
        snap_read.setTopology(inf::FieldAttributes::Unassigned(), gids, do_own);
        snap_read.load("dog.snap");

        snap_read.setMaxChunkSizeInMB(max_chunk_in_mb);
        auto in_field = snap_read.retrieve("some_field");
        auto end = std::chrono::system_clock::now();
        if (mp.Rank() == 0)
            std::cout << "Snap reading took: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
                      << " seconds" << std::endl;

        assert(in_field->size() == field->size());
        for (int i = 0; i < in_field->size(); i++) {
            std::vector<double> g(in_field->blockSize());
            in_field->value(i, g.data());
            if (g[0] != some_vector[i]) printf("ERROR: Snap file corrupted!%e != %e\n", g[0], some_vector[i]);
        }
    }

    if (mp.Rank() == 0) {
        std::cout << "Snap completed successfully" << std::endl;
    }

    MessagePasser::Finalize();
}
