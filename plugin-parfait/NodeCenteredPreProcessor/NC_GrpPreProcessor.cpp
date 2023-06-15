#include "NC_GrpPreProcessor.h"
#include "Distributor.h"
#include "../shared/ReaderFactory.h"
#include "PProcessor.h"
#include <t-infinity/SurfaceElementWinding.h>
#include <t-infinity/SurfaceNeighbors.h>
#include <t-infinity/Communicator.h>
#include <t-infinity/MeshSanityChecker.h>
#include <t-infinity/ReorderMesh.h>
#include <t-infinity/PancakeMeshAdapterTo.h>
#include <t-infinity/MeshHelpers.h>
#include <SerialPreProcessor/SerialPreProcessor.h>
#include "PancakePreProcessorAdapterTo.h"
#include "../shared/JsonSettings.h"

#include "PartitionGrouper.h"

std::shared_ptr<inf::MeshInterface> NC_GrpPreProcessor::load(MPI_Comm comm, std::string filename_or_json) const {
    MessagePasser mp(comm);

    bool cache_reordering = reorder_cache_efficiency;
    bool mesh_checks = run_sanity_checks;

    auto settings = PluginParfait::parseJsonSettings(filename_or_json);
    if (settings.count("use_cache_reordering")) {
        cache_reordering = settings.at("use_cache_reordering").asBool();
    }
    if (settings.count("use_mesh_checks")) {
        mesh_checks = settings.at("use_mesh_checks").asBool();
    }

    std::shared_ptr<inf::TinfMesh> mesh;
    if (mp.NumberOfProcesses() != 1) {
        std::string filename = settings.at("grid_filename").asString();
        auto reader = ReaderFactory::getReaderByFilename(filename);

        auto partitioner = NC::Partitioner::getPartitioner(mp, "default");
        NodeCentered::PProcessor pre_processor(mp, reader, *partitioner);
        mesh = pre_processor.createMesh();
    } else {
        SerialPreProcessor serial_pre_processor;
        auto the_mesh = serial_pre_processor.load(mp.getCommunicator(), settings.at("grid_filename").asString());
        mesh = std::dynamic_pointer_cast<inf::TinfMesh>(the_mesh);
    }

    if (mp.NumberOfProcesses() != 1) {
        if (cache_reordering) {
            mp_rootprint("NC_GrpPP: Reordering n2n for cache efficiency\n");
            mesh = inf::MeshReorder::reorderNodes(mesh);
        }
    } else {
        // This is needed for VULCAN, since we cannot reorder in serial when running
        // Vulcan.
        mp_rootprint("NC_GrpPP: Detected running in serial.  Disabling node reordering.\n");
    }

    if (not inf::is2D(mp, *mesh)) {
        auto surface_neighbors = SurfaceNeighbors::buildSurfaceNeighbors(mesh);
        SurfaceElementWinding::windAllSurfaceElementsOut(mp, *mesh.get(), surface_neighbors);
    }

    if (mesh_checks) {
        inf::MeshSanityChecker::checkAll(*mesh, mp);
    }

    PG::PartitionGrouper group_by_machine;
    int split_type = MPI_COMM_TYPE_SHARED;
    int key = 0;
    MPI_Info info;
    MPI_Comm machine_comm;
    int mpi_err = 0;
    mpi_err = MPI_Info_create(&info);
    mpi_err = MPI_Comm_split_type(comm, split_type, key, info, &machine_comm);
    if (mpi_err != 0) {
        std::cout << __FILE__ << " +" << __LINE__ << " mpi_err = " << mpi_err << std::endl;
    }

    int32_t num_real_comm_proccesses = 1;
    mpi_err = MPI_Comm_size(comm, &num_real_comm_proccesses);
    if (mpi_err != 0) {
        std::cout << __FILE__ << " +" << __LINE__ << " mpi_err = " << mpi_err << std::endl;
    }
    int32_t num_partitions_per_group = 1;
    mpi_err = MPI_Comm_size(machine_comm, &num_partitions_per_group);
    if (mpi_err != 0) {
        std::cout << __FILE__ << " +" << __LINE__ << " mpi_err = " << mpi_err << std::endl;
    }
    if (mp.Rank() == 0) {
        std::cout << "NC_GrpPP: num_real_comm_proccesses = " << num_real_comm_proccesses << std::endl;
        std::cout << "NC_GrpPP: num_partitions_per_group = " << num_partitions_per_group << std::endl;
    }
    auto grouped_mesh = group_by_machine.group(mp, *mesh, num_partitions_per_group);

    return grouped_mesh;
}
void NC_GrpPreProcessor::enableSanityChecks() { run_sanity_checks = true; }
void NC_GrpPreProcessor::disableSanityChecks() { run_sanity_checks = false; }
void NC_GrpPreProcessor::enableCacheReordering() { reorder_cache_efficiency = true; }
void NC_GrpPreProcessor::disableCacheReordering() { reorder_cache_efficiency = false; }

std::shared_ptr<inf::MeshLoader> createPreProcessor() { return std::make_shared<NC_GrpPreProcessor>(); }
