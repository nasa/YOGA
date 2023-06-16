#include <MessagePasser/MessagePasser.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <parfait/ToString.h>
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/GlobalPairIds.h>
#include <t-infinity/GlobalToLocal.h>
#include <parfait/Throw.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/MeshLoader.h>

std::vector<std::string> getArgsAsStrings(int argc, char** argv) {
    std::vector<std::string> arguments;
    for (int i = 0; i < argc; i++) {
        arguments.emplace_back(argv[i]);
    }
    return arguments;
}

long countNumberOfTotalFaces(MessagePasser mp, const std::map<long, std::array<long, 2>>& global_id_to_pair) {
    long max = 0;
    for (auto& p : global_id_to_pair) {
        long global = p.first;
        max = std::max(global, max);
    }
    return mp.ParallelMax(max);
}

long countTotalCells(MessagePasser mp, const inf::MeshInterface& mesh) {
    long cell_count = 0;
    for (int id = 0; id < mesh.cellCount(); id++) {
        if (mesh.ownedCell(id)) cell_count++;
    }
    return mp.ParallelSum(cell_count);
}

int main(int argc, char* argv[]) {
    MessagePasser::Init();
    MessagePasser mp(MPI_COMM_WORLD);

    if (argc < 2) {
        printf("Usage: To create global face ids from a mesh and write them to a file\n\n");
        exit(1);
    }

    std::vector<std::string> arguments = getArgsAsStrings(argc, argv);
    std::string filename = arguments[1];
    std::string plugin_name = "CC_ReProcessor";
    mp_rootprint("Using mesh pre-processor %s\n", plugin_name.c_str());
    auto pp = inf::getMeshLoader(inf::getPluginDir(), plugin_name);
    auto mesh_plugin = pp->load(mp.getCommunicator(), filename);
    auto& mesh = *mesh_plugin.get();

    auto face_neighbors = inf::FaceNeighbors::flattenToGlobals(mesh, inf::FaceNeighbors::build(mesh));

    auto g2l_cell = inf::GlobalToLocal::buildCell(mesh);
    auto global_pairs = inf::buildGlobalPairIds(mp.getCommunicator(), face_neighbors, countTotalCells(mp, mesh));
    std::map<long, std::array<long, 2>> global_pairs_inverted;
    for (auto& pair : global_pairs) {
        global_pairs_inverted[pair.second] = pair.first;
    }
    auto total_faces = countNumberOfTotalFaces(mp, global_pairs_inverted);
    printf("total_faces = %ld\n", total_faces);

    long chunk_size = total_faces / mp.NumberOfProcesses();
    long num_written = 0;
    std::vector<std::array<long, 2>> chunk_data;
    FILE* fp = nullptr;
    if (mp.Rank() == 0) {
        fp = fopen("pairs.out", "w");
        if (fp == nullptr) PARFAIT_THROW("Could not open file for writing: pairs.out");
    }
    while (num_written < total_faces) {
        long num_to_write = std::min(chunk_size, total_faces - num_written);
        long start = num_written;
        long end = start + num_to_write;
        chunk_data = mp.GatherByOrdinalRange(global_pairs_inverted, start, end, 0);
        if (mp.Rank() == 0) {
            long global_id = start;
            for (auto& pair : chunk_data) {
                fprintf(fp, "%ld %ld: %ld\n", pair[0], pair[1], global_id++);
            }
        }
        num_written += num_to_write;
    }
    MessagePasser::Finalize();
}
