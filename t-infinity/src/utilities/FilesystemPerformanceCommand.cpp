#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/AddMissingFaces.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/QuiltTags.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/FileStreamer.h>
#include <t-infinity/ReorderMesh.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/HangingEdgeRemesher.h>
#include <t-infinity/StitchMesh.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include "../t-infinity/MapbcLumper.h"
#include <parfait/Timing.h>
#include <Tracer.h>
#include <fcntl.h>

using namespace inf;

class FilesystemPerformanceCommand : public SubCommand {
  public:
    std::string description() const override { return "Test filesystem IO performance"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"chunk-size", "c"}, "Chunk size in MB", false, "10");
        m.addParameter({"total-size", "t"}, "Total size in GB", false, "10");
        m.addFlag({"no-mpi"}, "Do not use MPI funneling / broadcasting");
        m.addFlag({"no-filesystem", "no-fs"}, "Do not actually use filesystem (for MPI per checking)");
        m.addFlag({"skip-read"}, "Skip reading");
        m.addFlag({"skip-write"}, "Skip writing");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        size_t total_size_gb = m.getInt("total-size");
        size_t chunk_size_mb = m.getInt("chunk-size");
        size_t gb_to_mb = 1024;
        size_t mb_to_b = 1024 * 1024;
        size_t num_chunks = total_size_gb * gb_to_mb / chunk_size_mb;
        size_t num_doubles_per_chunk = chunk_size_mb * mb_to_b / sizeof(double);
        size_t avail_mem_system_mb = Tracer::availableMemoryMB();
        size_t used_mem_process_mb = Tracer::usedMemoryMB();
        size_t total_mem_system_mb = Tracer::totalMemoryMB();

        bool skip_mpi = m.has("no-mpi");
        bool skip_fs = m.has("no-filesystem");
        mp_rootprint("Test data sizes: \n");
        mp_rootprint("            total: %ld GB\n", total_size_gb);
        mp_rootprint("            chunk: %ld MB\n", chunk_size_mb);
        mp_rootprint("       num chunks: %ld\n", num_chunks);
        mp_rootprint("doubles per chunk: %ld\n", num_doubles_per_chunk);

        mp_rootprint("Initial memory profile: \n");
        mp_rootprint("         total system mem: %ld MB\n", total_mem_system_mb);
        mp_rootprint("         avail system mem: %ld MB\n", avail_mem_system_mb);
        mp_rootprint(" pre-alloc, process using: %ld MB\n", used_mem_process_mb);
        mp_rootprint("\n");

        int root = 0;

        Parfait::Stopwatch stopwatch;
        std::vector<double> buffer(num_doubles_per_chunk, 1.0);

        // write
        std::shared_ptr<FileStreamer> fp = nullptr;

        size_t max_process_mem_mb = used_mem_process_mb;
        size_t min_avail_system_mem_mb = avail_mem_system_mb;
        if (mp.Rank() == root and not skip_fs) {
            fp = inf::FileStreamer::create("default");
            if(not fp->openWrite("test.bin")) {
                mp_rootprint("Could not open test file for writing write-test.bin\n");
                mp.Abort(54321);
            }
        }
        for (size_t c = 0; c < num_chunks; c++) {
            std::vector<std::vector<double>> parallel_chunks;
            if (not skip_mpi) {
                parallel_chunks = mp.Gather(buffer, root);
            }
            max_process_mem_mb = std::max(max_process_mem_mb, Tracer::usedMemoryMB());
            min_avail_system_mem_mb = std::min(min_avail_system_mem_mb, Tracer::availableMemoryMB());
            if (mp.Rank() == 0) {
                for (auto& remote_chunk : parallel_chunks) {
                    for (size_t i = 0; i < remote_chunk.size(); i++) {
                        buffer[i] = remote_chunk[i];
                    }
                    max_process_mem_mb = std::max(max_process_mem_mb, Tracer::usedMemoryMB());
                    min_avail_system_mem_mb = std::min(min_avail_system_mem_mb, Tracer::availableMemoryMB());
                }
                if (not skip_fs) {
                    fp->write(buffer.data(), sizeof(double), buffer.size());
                    max_process_mem_mb = std::max(max_process_mem_mb, Tracer::usedMemoryMB());
                    min_avail_system_mem_mb = std::min(min_avail_system_mem_mb, Tracer::availableMemoryMB());
                }
                max_process_mem_mb = std::max(max_process_mem_mb, Tracer::usedMemoryMB());
                min_avail_system_mem_mb = std::min(min_avail_system_mem_mb, Tracer::availableMemoryMB());
            }
        }
        if (not skip_fs and mp.Rank() == root) {
            fp->close();
        }
        stopwatch.stop();
        mp_rootprint("Writing took %s, %f MB/s\n",
                     stopwatch.to_string().c_str(),
                     total_size_gb * gb_to_mb / stopwatch.seconds());
        mp_rootprint("Post Write memory profile: \n");
        mp_rootprint("     total system mem: %ld MB\n", total_mem_system_mb);
        mp_rootprint(" min avail system mem: %ld MB\n", min_avail_system_mem_mb);
        mp_rootprint("    max process using: %ld MB\n", max_process_mem_mb);
        mp_rootprint("\n");

        stopwatch.reset();
        // read
        if (mp.Rank() == 0 and not skip_fs) {
            fp = inf::FileStreamer::create("default");
            if(not fp->openRead("test.bin")) {
                mp_rootprint("Could not open test file for reading test.bin\n");
                mp.Abort(54321);
            }
        }
        for (size_t c = 0; c < num_chunks; c++) {
            if (mp.Rank() == root) {
                if (not skip_fs) {
                    fp->read(buffer.data(), sizeof(double), buffer.size());
                }
            }
            max_process_mem_mb = std::max(max_process_mem_mb, Tracer::usedMemoryMB());
            min_avail_system_mem_mb = std::min(min_avail_system_mem_mb, Tracer::availableMemoryMB());
            if (not skip_mpi) {
                mp.Broadcast(buffer, root);
            }
            max_process_mem_mb = std::max(max_process_mem_mb, Tracer::usedMemoryMB());
            min_avail_system_mem_mb = std::min(min_avail_system_mem_mb, Tracer::availableMemoryMB());
        }
        if (not skip_fs and mp.Rank() == root) {
            fp->close();
        }

        mp_rootprint("Reading took %s, %f MB/s\n",
                     stopwatch.to_string().c_str(),
                     total_size_gb * gb_to_mb / stopwatch.seconds());
        mp_rootprint("Post Read memory profile: \n");
        mp_rootprint("     total system mem: %ld MB\n", total_mem_system_mb);
        mp_rootprint(" min avail system mem: %ld MB\n", min_avail_system_mem_mb);
        mp_rootprint("    max process using: %ld MB\n", max_process_mem_mb);
        mp_rootprint("\n");
    }
};

CREATE_INF_SUBCOMMAND(FilesystemPerformanceCommand)
