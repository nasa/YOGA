#include "NC_PreProcessor.h"
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

std::shared_ptr<inf::MeshInterface> NC_PreProcessor::load(MPI_Comm comm, std::string filename_or_json) const {
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
    std::string partitioner_string = "default";
    if (settings.count("partitioner")) {
        partitioner_string = settings.at("partitioner").asString();
    }

    std::shared_ptr<inf::TinfMesh> mesh;
    if (mp.NumberOfProcesses() != 1) {
        std::string filename = settings.at("grid_filename").asString();
        auto reader = ReaderFactory::getReaderByFilename(filename);

        auto partitioner = NC::Partitioner::getPartitioner(mp, partitioner_string);
        NodeCentered::PProcessor pre_processor(mp, reader, *partitioner);
        mesh = pre_processor.createMesh();
    } else {
        SerialPreProcessor serial_pre_processor;
        auto the_mesh = serial_pre_processor.load(mp.getCommunicator(), settings.at("grid_filename").asString());
        mesh = std::dynamic_pointer_cast<inf::TinfMesh>(the_mesh);
    }

    if (mp.NumberOfProcesses() != 1) {
        if (cache_reordering) {
            mp_rootprint("NC_PP: Reordering n2n for cache efficiency\n");
            mesh = inf::MeshReorder::reorderNodesRCM(mesh);
        }
    } else {
        // This is needed for VULCAN, since we cannot reorder in serial when running
        // Vulcan.
        mp_rootprint("NC_PP: Detected running in serial.  Disabling node reordering.\n");
    }

    // Don't automatically wind surface elements for 2D meshes because that doesn't make sense.
    // Also, if we're running in serial, the serial pre-processor already wound them, so don't wind them again.
    if (not inf::is2D(mp, *mesh) and mp.NumberOfProcesses() > 1) {
        auto surface_neighbors = SurfaceNeighbors::buildSurfaceNeighbors(mesh);
        SurfaceElementWinding::windAllSurfaceElementsOut(mp, *mesh, surface_neighbors);
    }

    if (mesh_checks) {
        inf::MeshSanityChecker::checkAll(*mesh, mp);
    }

    return mesh;
}
void NC_PreProcessor::enableSanityChecks() { run_sanity_checks = true; }
void NC_PreProcessor::disableSanityChecks() { run_sanity_checks = false; }
void NC_PreProcessor::enableCacheReordering() { reorder_cache_efficiency = true; }
void NC_PreProcessor::disableCacheReordering() { reorder_cache_efficiency = false; }

std::shared_ptr<inf::MeshLoader> createPreProcessor() { return std::make_shared<NC_PreProcessor>(); }
