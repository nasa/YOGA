#include "CellCenteredReProcessor.h"

#include <NodeCenteredPreProcessor/NC_PreProcessor.h>
#include <Tracer.h>
#include <parfait/RecursiveBisection.h>
#include <parfait/Dictionary.h>
#include <t-infinity/SetNodeOwners.h>
#include <t-infinity/Cell.h>
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/GhostSyncer.h>
#include <t-infinity/MeshSanityChecker.h>
#include <t-infinity/BetterDistanceCalculator.h>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/MeshShuffle.h>
#include <t-infinity/TinfMeshAppender.h>
#include <t-infinity/SurfaceElementWinding.h>
#include <t-infinity/SurfaceNeighbors.h>
#include <t-infinity/ReorderMesh.h>
#include <t-infinity/PartitionDiffusion.h>
#include <t-infinity/AddHalo.h>
#include <parfait/JsonParser.h>
#include <parfait/GraphOrderings.h>
#include <SerialPreProcessor/SerialPreProcessor.h>
#include "../shared/JsonSettings.h"
#ifdef HAVE_PARMETIS
#include "../shared/ParmetisWrapper.h"
#endif

using namespace inf;

std::vector<Parfait::Point<double>> buildOwnedVolumeCellCenters(inf::MeshInterface& mesh, bool hiro_centroid = false) {
    std::vector<Parfait::Point<double>> cell_centers;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.is3DCellType(mesh.cellType(c)) and mesh.ownedCell(c)) {
            inf::Cell cell(mesh, c);
            if (hiro_centroid) {
                cell_centers.push_back(cell.nishikawaCentroid());
            } else {
                cell_centers.push_back(cell.averageCenter());
            }
        }
    }
    return cell_centers;
}

std::vector<double> buildOwnedVolumeWeights(inf::MeshInterface& mesh,
                                            const std::map<inf::MeshInterface::CellType, double>& type_to_weights) {
    std::vector<double> weights;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.is3DCellType(mesh.cellType(c)) and mesh.ownedCell(c)) {
            weights.push_back(type_to_weights.at(mesh.cellType(c)));
        }
    }
    return weights;
}

std::vector<int> putOwnedVolumeCellPartIdsIntoListOfFullCells(const inf::MeshInterface& mesh,
                                                              const std::vector<int>& owned_volume_cell_part_ids) {
    std::vector<int> assigned_owner(mesh.cellCount(), -1);
    int index = 0;
    for (int c = 0; c < mesh.cellCount(); c++) {
        if (mesh.is3DCellType(mesh.cellType(c)) and mesh.ownedCell(c)) {
            assigned_owner[c] = owned_volume_cell_part_ids[index++];
        }
    }
    return assigned_owner;
}

std::map<int, std::unique_ptr<TinfMeshAppender>> queueFragmentsToSend(const std::shared_ptr<MeshInterface>& mesh,
                                                                      const std::vector<int>& new_cell_owners,
                                                                      std::vector<std::vector<int>>&& c2c) {
    std::map<int, std::set<int>> fragment_cell_ids;
    for (int c = 0; c < mesh->cellCount(); c++) {
        if (mesh->is3DCellType(mesh->cellType(c))) {
            int new_owner = new_cell_owners[c];
            fragment_cell_ids[new_owner].insert(c);
            for (auto neighbor : c2c[c]) {
                fragment_cell_ids[new_owner].insert(neighbor);
            }
        }
    }

    c2c.clear();
    c2c.shrink_to_fit();

    std::map<int, std::unique_ptr<TinfMeshAppender>> fragments;
    for (auto& pair : fragment_cell_ids) {
        int target = pair.first;
        auto& cell_ids = pair.second;
        for (auto c : cell_ids) {
            TinfMeshCell cell(*mesh, c);
            cell.owner = new_cell_owners[c];
            if (fragments.count(target) == 0) {
                fragments[target] = std::make_unique<TinfMeshAppender>();
            }
            fragments.at(target)->addCell(cell);
        }
    }
    return fragments;
}

std::map<int, MessagePasser::Message> packFragments(std::map<int, std::unique_ptr<TinfMeshAppender>>& fragments) {
    std::map<int, MessagePasser::Message> messages;
    for (auto& pair : fragments) {
        auto& frag = pair.second->getData();
        frag.pack(messages[pair.first]);
        pair.second.reset();
    }
    return messages;
}

std::vector<int> assignOwnersToAllCellsFromParmetis(MessagePasser mp, std::shared_ptr<MeshInterface> mesh) {
    mp_rootprint("CC_RP: forming inf to ParMETIS global ID mapping\n");
    auto volume_filter = inf::FilterFactory::volumeFilter(mp.getCommunicator(), mesh);
    auto volume_mesh = volume_filter->getMesh();
    std::vector<long> cell_l2g = inf::LocalToGlobal::buildCell(*volume_mesh);
    std::vector<bool> do_own = inf::extractDoOwnCells(*volume_mesh);
    auto inf_to_parmetis_global_ordering =
        Parfait::GraphOrderings::buildDistributedToContiguous(mp.getCommunicator(), cell_l2g, do_own);
    auto local_inf_to_parmetis = Parfait::GraphOrderings::buildLocalToCompactOwned(do_own);
    auto parmetis_to_inf_local = Parfait::MapTools::invert(local_inf_to_parmetis);

    mp_rootprint("CC_RP: forming global cell to cell graph\n");
    auto c2c = inf::CellToCell::build(*volume_mesh);
    int num_owned = volume_mesh->countOwnedCells();
    std::vector<std::vector<long>> c2c_global(num_owned);
    for (int c = 0; c < volume_mesh->cellCount(); c++) {
        if (not volume_mesh->ownedCell(c)) continue;
        auto c_parmetis = local_inf_to_parmetis.at(c);
        for (int& neighbor : c2c[c]) {
            auto neighbor_petsc = inf_to_parmetis_global_ordering.at(volume_mesh->globalCellId(neighbor));
            c2c_global[c_parmetis].push_back(neighbor_petsc);
        }
    }

    std::vector<int> new_cell_owners_parmetis_local_ordering;
#ifdef HAVE_PARMETIS
    mp_rootprint("CC_RP: Calling ParMETIS\n");
    new_cell_owners_parmetis_local_ordering = Parfait::ParmetisWrapper::generatePartVector(mp, c2c_global);
    mp_rootprint("CC_RP: Remap back ParMETIS\n");
#else
    return {};
#endif
    std::vector<int> new_cell_owners_volume(volume_mesh->cellCount(), -1);
    for (int c = 0; c < volume_mesh->cellCount(); c++) {
        if (volume_mesh->ownedCell(c)) {
            new_cell_owners_volume[c] = new_cell_owners_parmetis_local_ordering.at(local_inf_to_parmetis.at(c));
        }
    }
    std::vector<int> new_cell_owners(mesh->cellCount(), -1);
    for (int c = 0; c < volume_mesh->cellCount(); c++) {
        int orig_cell = volume_mesh->previousCellId(c);
        new_cell_owners[orig_cell] = new_cell_owners_volume[c];
    }

    mp_rootprint("CC_RP: Build syncer\n");
    GhostSyncer syncer(mp, mesh);
    syncer.initializeCellSyncing();
    mp_rootprint("CC_RP: sync\n");
    syncer.syncCells(new_cell_owners);
    mp_rootprint("CC_RP: done sync\n");

    auto c2c_surface_only = inf::FaceNeighbors::buildOnlyForCellsWithDimensionality(*mesh, 2);
    mp.Barrier();
    mp_rootprint("CC_RP: using c2f\n");
    for (int c = 0; c < mesh->cellCount(); c++) {
        if (mesh->is2DCell(c)) {
            if (c2c_surface_only[c].empty()) {
                PARFAIT_THROW("Missing a face for a surface element\n");
            }
            new_cell_owners[c] = new_cell_owners[c2c_surface_only[c][0]];
        }
    }

    return new_cell_owners;
}

std::vector<int> assignOwnersToAllCells(MessagePasser mp,
                                        std::shared_ptr<MeshInterface> mesh,
                                        bool use_hiro_centroid = false) {
    mp_rootprint("CC_RP: get cell centers\n");
    auto owned_volume_cell_centers = buildOwnedVolumeCellCenters(*mesh, use_hiro_centroid);

    std::map<inf::MeshInterface::CellType, double> type_weights;
    type_weights[inf::MeshInterface::TETRA_4] = 4;
    type_weights[inf::MeshInterface::PYRA_5] = 5;
    type_weights[inf::MeshInterface::PENTA_6] = 5;
    type_weights[inf::MeshInterface::HEXA_8] = 6;
    mp_rootprint("CC_RP: Using cell weights:\n");
    mp_rootprint("CC_RP:      Tet:      4.0\n");
    mp_rootprint("CC_RP:      Pyramid:  5.0\n");
    mp_rootprint("CC_RP:      Prism:    5.0\n");
    mp_rootprint("CC_RP:      Hex:      6.0\n");
    auto owned_volume_cell_weights = buildOwnedVolumeWeights(*mesh, type_weights);
    mp_rootprint("CC_RP: RCB\n");
    Tracer::begin("RCB");
    Tracer::traceMemory();
    auto new_cell_owners = Parfait::recursiveBisection(
        mp, std::move(owned_volume_cell_centers), std::move(owned_volume_cell_weights), mp.NumberOfProcesses(), 1.0e-4);
    Tracer::traceMemory();
    Tracer::end("RCB");

    mp_rootprint("CC_RP: set owners of owned volume cells\n");
    new_cell_owners = putOwnedVolumeCellPartIdsIntoListOfFullCells(*mesh, new_cell_owners);

    mp_rootprint("CC_RP: Build syncer\n");
    GhostSyncer syncer(mp, mesh);
    syncer.initializeCellSyncing();
    mp_rootprint("CC_RP: sync\n");
    syncer.syncCells(new_cell_owners);
    mp_rootprint("CC_RP: done sync\n");

    auto c2c_surface_only = inf::FaceNeighbors::buildOnlyForCellsWithDimensionality(*mesh, 2);
    mp.Barrier();
    mp_rootprint("CC_RP: using c2f\n");
    for (int c = 0; c < mesh->cellCount(); c++) {
        if (mesh->is2DCell(c)) {
            if (c2c_surface_only[c].empty()) {
                PARFAIT_THROW("Missing a face for a surface element\n");
            }
            new_cell_owners[c] = new_cell_owners[c2c_surface_only[c][0]];
        }
    }

    return new_cell_owners;
}

std::map<int, std::unique_ptr<TinfMeshAppender>> queueAppenders(MessagePasser mp,
                                                                std::shared_ptr<inf::MeshInterface>& mesh,
                                                                const std::vector<int>& new_cell_owners) {
    mp_rootprint("CC_RP: building c2c\n");
    Tracer::begin("build c2c");
    Tracer::traceMemory();
    auto c2c = inf::CellToCell::build(*mesh);
    Tracer::traceMemory();
    Tracer::end("build c2c");

    mp_rootprint("CC_RP: building send fragments\n");
    Tracer::begin("queue fragments");
    Tracer::traceMemory();
    auto fragment_appenders = queueFragmentsToSend(mesh, new_cell_owners, std::move(c2c));
    mesh.reset();
    Tracer::traceMemory();
    Tracer::end("queue fragments");
    return fragment_appenders;
}

std::map<int, MessagePasser::Message> packFragments(MessagePasser mp,
                                                    std::shared_ptr<inf::MeshInterface>& mesh,
                                                    const std::vector<int>& new_cell_owners) {
    auto fragment_appenders = queueAppenders(mp, mesh, new_cell_owners);

    mp_rootprint("CC_RP: packing send fragments\n");
    Tracer::begin("pack fragments");
    Tracer::traceMemory();
    auto fragments = packFragments(fragment_appenders);
    fragment_appenders.clear();
    Tracer::traceMemory();
    Tracer::end("pack fragments");
    return fragments;
}

TinfMeshAppender unpackParallelFragments(MessagePasser mp,
                                         std::shared_ptr<inf::MeshInterface>& mesh,
                                         const std::vector<int>& new_cell_owners) {
    auto fragments = packFragments(mp, mesh, new_cell_owners);

    mp_rootprint("CC_RP: Exchanging fragments\n");
    Tracer::begin("Exchange");
    Tracer::traceMemory();
    fragments = mp.Exchange(fragments);
    Tracer::traceMemory();
    Tracer::end("Exchange");

    mp_rootprint("CC_RP: Merging fragments\n");
    Tracer::begin("Merge fragments");
    Tracer::traceMemory();
    TinfMeshAppender final_mesh;
    for (auto& pair : fragments) {
        TinfMeshData frag_data;
        frag_data.unpack(pair.second);
        TinfMesh frag(std::move(frag_data), mp.Rank());
        for (int c = 0; c < frag.cellCount(); c++) {
            final_mesh.addCell(frag.getCell(c));
        }
    }
    Tracer::traceMemory();
    Tracer::end("Merge fragments");
    return final_mesh;
}

std::shared_ptr<TinfMesh> shuffleCellsFromNodeBasedPartitioning(MessagePasser mp,
                                                                std::shared_ptr<inf::MeshInterface>& mesh,
                                                                const std::vector<int>& new_cell_owners) {
    auto final_mesh = unpackParallelFragments(mp, mesh, new_cell_owners);
    mp_rootprint("CC_RP: creating mesh\n");
    Tracer::begin("create mesh");
    Tracer::traceMemory();
    auto out_mesh = std::make_shared<TinfMesh>(final_mesh.getData(), mp.Rank());
    Tracer::traceMemory();
    Tracer::end("create mesh");
    setNodeOwners(mp, out_mesh);
    return out_mesh;
}

std::shared_ptr<MeshInterface> CC_ReProcessor::load(MPI_Comm comm, std::string grid_filename_or_json) const {
    auto mp = MessagePasser(comm);

    auto settings = PluginParfait::parseJsonSettings(grid_filename_or_json);
    if (settings.count("use_cache_reordering")) {
        reorder_for_efficiency = settings.at("use_cache_reordering").asBool();
    }
    if (settings.count("use_random_reordering")) {
        reorder_randomly = settings.at("use_random_reordering").asBool();
    }
    if (settings.count("use_q_reordering")) {
        use_q_reordering = settings.at("use_q_reordering").asBool();
    }
    if (settings.count("q_reordering_prune_width")) {
        q_reordering_prune_width = settings.at("q_reordering_prune_width").asDouble();
    }
    if (settings.count("use_mesh_checks")) {
        run_sanity_checks = settings.at("use_mesh_checks").asBool();
    }
    if (settings.count("use_hiro_centroid")) {
        use_hiro_centroid = settings.at("use_hiro_centroid").asBool();
    }
    if (settings.count("use_partition_diffusion")) {
        use_partition_diffusion = settings.at("use_partition_diffusion").asBool();
    }
    if (settings.count("partitioner")) {
        partitioner = Parfait::StringTools::tolower(settings.at("partitioner").asString());
        if (partitioner == "parmetis") {
#ifndef HAVE_PARMETIS
            PARFAIT_WARNING(
                "ParMETIS partitioner was requested.  But Parfait was not configured with ParMETIS "
                "support");
            partitioner = "default";
#endif
        }
    }
    std::string grid_filename = settings.at("grid_filename").asString();

    mp_rootprint("CC_RP settings: %s\n", settings.dump(2).c_str());
    mp_rootprint("CC_RP: use_cache_reordering = %s\n", (reorder_for_efficiency) ? ("true") : ("false"));
    mp_rootprint("CC_RP: run_sanity_checks = %s\n", (run_sanity_checks) ? ("true") : ("false"));
    mp_rootprint("CC_RP: use_hiro_centroid = %s\n", (use_hiro_centroid) ? ("true") : ("false"));
    mp_rootprint("CC_RP: use_partition_diffusion = %s\n", (use_partition_diffusion) ? ("true") : ("false"));

    // we're going to run our own mesh checks and reordering, don't run them twice.
    settings["use_mesh_checks"] = false;
    settings["use_cache_reordering"] = false;
    settings["partitioner"] = "default";

    std::shared_ptr<TinfMesh> out_mesh;
    if (mp.NumberOfProcesses() != 1) {
        NC_PreProcessor nc_pp;
        mp_rootprint("CC_RP: loading from NC_PP\n");
        Tracer::traceMemory();
        Tracer::begin("NC_PP");
        Tracer::traceMemory();
        auto mesh = nc_pp.load(comm, settings.dump());
        Tracer::traceMemory();
        Tracer::end("NC_PP");

        Tracer::begin("addHaloNodes");
        Tracer::traceMemory();
        mesh = inf::addHaloNodes(mp, *mesh);
        Tracer::traceMemory();
        Tracer::end("addHaloNodes");

        mp_rootprint("CC_RP: assigning new owners\n");
        std::vector<int> new_cell_owners;
        Tracer::begin("assignOwners");
        Tracer::traceMemory();
        new_cell_owners = assignOwnersToAllCells(comm, mesh, use_hiro_centroid);
        Tracer::traceMemory();
        Tracer::end("assignOwners");

        Tracer::begin("shuffle node to cell");
        Tracer::traceMemory();
        out_mesh = shuffleCellsFromNodeBasedPartitioning(mp, mesh, new_cell_owners);
        Tracer::traceMemory();
        Tracer::end("shuffle node to cell");

        if (partitioner == "parmetis") {
            mp_rootprint("CC_RP: repartitioning with ParMETIS\n");
            new_cell_owners = assignOwnersToAllCellsFromParmetis(comm, out_mesh);
            mp_rootprint("CC_RP: shuffling to match ParMETIS cell assignment\n");
            out_mesh = inf::MeshShuffle::shuffleCells(mp, *out_mesh, new_cell_owners);
        }
        if (settings.isTrue("use rcb4d")) {
            mp_rootprint("CC_RP: repartitioning with RCB4D\n");
            std::vector<double> distance, time;
            std::vector<int> tag_as_int;
            auto tags = settings.at("tags").asInts();
            std::set<int> tags_set(tags.begin(), tags.end());
            std::tie(distance, time, tag_as_int) =
                calcBetterDistance(mp, *out_mesh, tags_set, inf::extractPoints(*out_mesh));
            for (auto& d : distance) {
                d *= 5;
            }
            auto new_node_owners = inf::MeshShuffle::repartitionNodes4D(comm, *out_mesh, distance);
            mp_rootprint("CC_RP: shuffling to match RCB4D node assignment\n");
            out_mesh = inf::MeshShuffle::shuffleNodes(mp, *out_mesh, new_node_owners);
        }
    } else {
        SerialPreProcessor serial_pre_processor;
        auto the_mesh = serial_pre_processor.load(mp.getCommunicator(), settings.at("grid_filename").asString());
        out_mesh = std::dynamic_pointer_cast<TinfMesh>(the_mesh);
    }

    if (use_partition_diffusion) {
        out_mesh = inf::PartitionDiffusion::cellBasedDiffusion(mp, out_mesh, 10);
    }

    if (mp.NumberOfProcesses() != 1) {
        if (reorder_randomly) {
            mp_rootprint("CC_RP: Reordering for randomly for linear solver stability\n");
            out_mesh = inf::MeshReorder::reorderCellsRandomly(out_mesh);
            out_mesh = inf::MeshReorder::reorderNodesBasedOnCells(out_mesh);

        } else if (use_q_reordering) {
            mp_rootprint("CC_RP: Reordering cells based on Q reordering with prune width %lf\n",
                         q_reordering_prune_width);
            out_mesh = inf::MeshReorder::reorderCellsQ(out_mesh, q_reordering_prune_width);
            out_mesh = inf::MeshReorder::reorderNodesBasedOnCells(out_mesh);
        } else if (reorder_for_efficiency) {
            mp_rootprint("CC_RP: Reordering faces for cache efficiency\n");
            mp_rootprint("CC_RP: Before cache optimization: \n");
            inf::MeshReorder::reportEstimatedCacheEfficiencyForCells(mp, *out_mesh);
            out_mesh = inf::MeshReorder::reorderCells(out_mesh);
            mp_rootprint("CC_RP: Reordering node to cell for cache efficiency\n");
            out_mesh = inf::MeshReorder::reorderNodesBasedOnCells(out_mesh);
            mp_rootprint("CC_RP: After cache optimization: \n");
            inf::MeshReorder::reportEstimatedCacheEfficiencyForCells(mp, *out_mesh);
        }
    } else {
        mp_rootprint(
            "CC_RP: Detected running in serial.  Disabling cell reordering since we cannot reorder "
            "in serial when "
            "running Vulcan\n");
    }
    mp_rootprint("CC_RP: done\n");
    mp_rootprint("CC_RP: Building surface neighbors for winding\n");
    Tracer::begin("surface neighbors");
    Tracer::traceMemory();
    auto surface_neighbors = SurfaceNeighbors::buildSurfaceNeighbors(out_mesh);
    Tracer::traceMemory();
    Tracer::end("surface neighbors");
    mp_rootprint("CC_RP: Winding surface elements out\n");
    Tracer::begin("windSurface");
    Tracer::traceMemory();
    SurfaceElementWinding::windAllSurfaceElementsOut(mp, *out_mesh, surface_neighbors);
    Tracer::traceMemory();
    Tracer::end("windSurface");

    if (run_sanity_checks) {
        mp_rootprint("CC_RP: Running mesh sanity checks\n");
        inf::MeshSanityChecker::checkAll(*out_mesh, mp);
    }

    if (mp.NumberOfProcesses() > 1) {
        // will throw floating point exception if run in "serial"
        inf::LoadBalance::printUnweightedCellStatistics(mp, *out_mesh);
    }
    return out_mesh;
}

std::shared_ptr<MeshLoader> createPreProcessor() { return std::make_shared<CC_ReProcessor>(); }
