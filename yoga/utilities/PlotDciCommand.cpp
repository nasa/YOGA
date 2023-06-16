#include <stdio.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include "DcifReader.h"
#include "DcifChecker.h"
#include <parfait/UgridReader.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/MeshHelpers.h>
#include <parfait/LinearPartitioner.h>
#include <parfait/SyncField.h>
#include <t-infinity/GlobalToLocal.h>
#include <SuggarDciReader.h>
#include "DcifDistributor.h"
#include <parfait/Checkpoint.h>
using namespace inf;
using namespace YOGA;

class PlotDciCommand : public SubCommand {
  public:
    std::string description() const override { return "Plot assembly from Suggar++ *.dci or FUN3D *.dcif files"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(), Help::help());
        m.addParameter(Alias::inputFile(), Help::inputFile(), true);
        m.addParameter(Alias::mesh(),Help::mesh(),true);
        m.addParameter({"o"},"Output filename",false,"dci.vtk");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto grid_filename = m.get(inf::Alias::mesh());
        auto mesh = inf::shortcut::loadMesh(mp,grid_filename);

        auto fun3d_dci = std::make_shared<Dcif::FlattenedDomainConnectivity>(0);
        std::shared_ptr<SuggarDciReader> suggar_dci = nullptr;
        std::vector<int> global_iblank_on_root;
        std::vector<long> grid_node_id_offsets;
        long total_receptors=0;
        std::function<std::vector<Dcif::Receptor>(long,long)> getReceptorsInRange;
        long global_node_count=0;
        auto filename = m.get(Alias::inputFile());
        auto extension = Parfait::StringTools::getExtension(filename);
        if("dcif" == extension) {
            importFromDcif(filename,
                           mp,
                           fun3d_dci,
                           mesh,
                           global_iblank_on_root,
                           grid_node_id_offsets,
                           total_receptors,
                           getReceptorsInRange,
                           global_node_count);
        }else if("dci" == extension){
            importFromSuggarDci(filename,
                                mp,
                                suggar_dci,
                                mesh,
                                global_iblank_on_root,
                                grid_node_id_offsets,
                                total_receptors,
                                getReceptorsInRange,
                                global_node_count);
        }
        else{
            throw std::logic_error("Unexpected file extension: "+extension);
        }

        auto resident_receptors = naivelyPartitionReceptors(mp, getReceptorsInRange, total_receptors, global_node_count);
        auto resident_iblank = naivelyPartitionNodeStatuses(mp, global_iblank_on_root, global_node_count);

        auto node_statuses = transferNodeStatusesToOwners(mp, global_node_count, resident_iblank, mesh);

        auto my_receptors = transferReceptorsToOwners(mp, mesh, global_node_count, resident_receptors, node_statuses);
        auto component_grid_id_for_each_node = getComponentGridIdsForNodes(mesh, grid_node_id_offsets);
        auto ghost_sync_pattern = buildNodeSyncPattern(mp,*mesh);
        auto g2l= GlobalToLocal::buildNode(*mesh);
        Parfait::syncVector(mp,node_statuses,g2l,ghost_sync_pattern);
        Parfait::syncVector(mp,component_grid_id_for_each_node,g2l,ghost_sync_pattern);

        auto iblank_field = std::make_shared<inf::VectorFieldAdapter>("iblank",inf::FieldAttributes::Node(),node_statuses);
        auto component_id_field = std::make_shared<inf::VectorFieldAdapter>("component_id",inf::FieldAttributes::Node(),component_grid_id_for_each_node);
        shortcut::visualize(m.get(Alias::outputFileBase()),mp,mesh,{iblank_field,component_id_field});
    }
    void importFromDcif(const std::string& filename,
                        MessagePasser& mp,
                        std::shared_ptr<Dcif::FlattenedDomainConnectivity>& dcif,
                        std::shared_ptr<inf::MeshInterface>& mesh,
                        std::vector<int>& global_iblank_on_root,
                        std::vector<long>& grid_node_id_offsets,
                        long& total_receptors,
                        std::function<std::vector<Dcif::Receptor>(long, long)>& getReceptorsInRange,
                        long& global_node_count) {
        global_node_count= globalNodeCount(mp,*mesh);
        std::shared_ptr<DcifReader> dcif_reader = nullptr;
        if(0 == mp.Rank()) {
            dcif_reader = std::make_shared<DcifReader>(filename);
            dcif = std::make_shared<Dcif::FlattenedDomainConnectivity>(dcif_reader->gridCount());
            dcif->setReceptorIds(dcif_reader->getFringeIds());
            dcif->setDonorCounts(dcif_reader->getDonorCounts());
            dcif->setDonorIds(dcif_reader->getDonorIds());
            dcif->setDonorWeights(dcif_reader->getDonorWeights());
            dcif->setIBlank(dcif_reader->getIblank());
            global_iblank_on_root = dcif->getIblank();
        }
        auto header = getDcifHeader(mp,dcif_reader);
        total_receptors = header.nfringes;
        grid_node_id_offsets = header.grid_node_id_offsets;
        PARFAIT_ASSERT(global_node_count == header.nnodes, "Dcif nnodes doesn't match grid");

        getReceptorsInRange = [&](long a,long b){
            return Dcif::getChunkOfReceptors(*dcif,a, b);
        };
    }
    void importFromSuggarDci(std::string filename,
                             MessagePasser mp,
                             std::shared_ptr<SuggarDciReader>& suggar_dci,
                             std::shared_ptr<MeshInterface> mesh,
                             std::vector<int>& global_iblank_on_root,
                             std::vector<long>& grid_gid_offsets,
                             long& receptor_count,
                             std::function<std::vector<Dcif::Receptor>(long, long)>& getReceptorsInRange,
                             long& global_node_count) {
        suggar_dci = std::make_shared<SuggarDciReader>(filename);
        global_iblank_on_root = suggar_dci->getNodeStatuses();
        grid_gid_offsets = suggar_dci->getGridOffsets();
        mp.Broadcast(grid_gid_offsets,0);
        receptor_count = suggar_dci->receptorCount();
        mp.Broadcast(receptor_count,0);
        global_node_count = grid_gid_offsets.back();
        getReceptorsInRange = [&](long a,long b){
            return suggar_dci->getReceptors(a,b);
        };
    }
    std::vector<int> getComponentGridIdsForNodes(const std::shared_ptr<inf::MeshInterface>& mesh,
                                                 const std::vector<long>& grid_node_id_offsets) const {
        std::vector<int> component_grid_id_for_each_node(mesh->nodeCount(),0);
        for(int node_id =0; node_id <mesh->nodeCount(); node_id++){
            long gid = mesh->globalNodeId(node_id);
            long offset = 0;
            bool is_set = false;
            for(int i=0;i<int(grid_node_id_offsets.size());i++){
                offset = grid_node_id_offsets[i];
                if(gid < offset) {
                    component_grid_id_for_each_node[node_id] = i;
                    is_set = true;
                    break;
                }
            }
            PARFAIT_ASSERT(is_set,"gid too big "+std::to_string(gid)+" offset: "+std::to_string(offset));
        }
        return component_grid_id_for_each_node;
    }

    std::vector<Dcif::Receptor> transferReceptorsToOwners(MessagePasser& mp,
                                                          std::shared_ptr<inf::MeshInterface> mesh,
                                                          long global_node_count,
                                                          const std::vector<Dcif::Receptor>& resident_receptors,
                                                          const std::vector<int>& node_statuses) const {
        std::map<int,std::vector<long>> rank_to_requested_receptor_ids;
        for(int i=0;i<mesh->nodeCount();i++){
            if(node_statuses[i] == -1) {
                auto gid = mesh->globalNodeId(i);
                int rank_who_has_it_resident = Parfait::LinearPartitioner::getWorkerOfWorkItem(gid,global_node_count,mp.NumberOfProcesses());
                rank_to_requested_receptor_ids[rank_who_has_it_resident].push_back(gid);
            }
        }
        return Dcif::exchangeReceptors(mp,resident_receptors, rank_to_requested_receptor_ids);
    }

    std::vector<int> transferNodeStatusesToOwners(const MessagePasser& mp,
                                   long global_node_count,
                                   std::vector<int>& resident_iblank,
                                   std::shared_ptr<inf::MeshInterface> mesh) const {
        auto g2l= GlobalToLocal::buildNode(*mesh);
        auto sync_pattern = buildReceptorSyncPattern(mp, mesh, global_node_count);
        std::map<long,int> naive_g2l;
        auto range = Parfait::LinearPartitioner::getRangeForWorker(mp.Rank(),global_node_count,mp.NumberOfProcesses());
        auto n = range.end - range.start;
        for(int i=0;i<int(n);i++)
            naive_g2l[range.start+i] = i;
        std::vector<int> node_statuses(mesh->nodeCount(),0);
        Parfait::StridedSyncer<int, decltype(g2l)> syncer(mp,resident_iblank.data(),node_statuses.data(),
                                                          sync_pattern,naive_g2l,g2l,1);
        syncer.start();
        syncer.finish();

        PARFAIT_ASSERT(int(node_statuses.size()) == mesh->nodeCount(), "Sizes don't match");
        return node_statuses;
    }
    Parfait::SyncPattern buildReceptorSyncPattern(const MessagePasser& mp,
                                                  const std::shared_ptr<inf::MeshInterface>& mesh,
                                                  long global_node_count) const {
        std::vector<long> have,need;
        std::vector<int> need_from;
        for(int i=0;i<mesh->nodeCount();i++) {
            if (mp.Rank() == mesh->nodeOwner(i)) {
                auto gid = mesh->globalNodeId(i);
                need.push_back(gid);
                auto owning_rank_in_naive_partition = Parfait::LinearPartitioner::getWorkerOfWorkItem(gid,global_node_count,mp.NumberOfProcesses());
                need_from.push_back(int(owning_rank_in_naive_partition));
            }
        }
        auto my_naive_range = Parfait::LinearPartitioner::getRangeForWorker(mp.Rank(),global_node_count,mp.NumberOfProcesses());
        for(auto gid =my_naive_range.start; gid <my_naive_range.end; gid++)
            have.push_back(gid);
        return Parfait::SyncPattern(mp,have,need,need_from);
    }

  private:
    struct DcifHeader{
        long nnodes;
        long nfringes;
        long ndonors;
        int ngrids;
        std::vector<long> grid_node_id_offsets;
    };

    DcifHeader getDcifHeader(MessagePasser mp, std::shared_ptr<DcifReader> reader) {
        DcifHeader header;
        if(0 == mp.Rank()){
            header.nnodes = reader->globalNodeCount();
            header.nfringes = reader->receptorCount();
            header.ndonors = reader->donorCount();
            header.ngrids = reader->gridCount();
            header.grid_node_id_offsets.push_back(0);
            for(int i=0;i<header.ngrids;i++) {
                auto previous_offset = header.grid_node_id_offsets.back();
                header.grid_node_id_offsets.push_back(previous_offset+reader->nodeCountForComponentGrid(i));
            }
        }
        mp.Broadcast(header.nnodes,0);
        mp.Broadcast(header.nfringes,0);
        mp.Broadcast(header.ndonors,0);
        mp.Broadcast(header.ngrids,0);
        mp.Broadcast(header.grid_node_id_offsets,0);
        return header;
    }

    std::vector<Dcif::Receptor> naivelyPartitionReceptors(
        const MessagePasser& mp,
        std::function<std::vector<Dcif::Receptor>(long,long)> getReceptorsInRange,
        long total_receptors,
        long global_node_count) const {
        std::vector<Dcif::Receptor> resident_receptors;
        int receptors_per_chunk = 5000;
        long chunks = total_receptors / receptors_per_chunk + 1;
        for(long chunk=0; chunk <chunks; chunk++){
            std::map<int,MessagePasser::Message> receptor_msgs_to_ranks;
            if(0 == mp.Rank()){
                auto range = Parfait::LinearPartitioner::getRangeForWorker(chunk,total_receptors,chunks);
                auto receptors = getReceptorsInRange(range.start, range.end);
                auto receptors_to_ranks = Dcif::mapReceptorsToRanks(receptors,global_node_count,mp.NumberOfProcesses());
                for(auto& pair:receptors_to_ranks){
                    int target_rank = pair.first;
                    auto& receptors_to_pack = pair.second;
                    auto& msg = receptor_msgs_to_ranks[target_rank];
                    msg.pack(long(receptors_to_pack.size()));
                    for(auto& receptor:pair.second)
                        receptor.pack(msg);
                }
            }
            receptor_msgs_to_ranks = mp.Exchange(receptor_msgs_to_ranks);
            for(auto& pair:receptor_msgs_to_ranks){
                auto& msg = pair.second;
                auto unpacked_receptors = Dcif::unpackReceptors(msg);
                for(auto& r:unpacked_receptors)
                    resident_receptors.emplace_back(r);
            }
        }
        return resident_receptors;
    }

    std::vector<int> naivelyPartitionNodeStatuses(const MessagePasser& mp,
                                                   const std::vector<int>& global_iblank_on_root,
                                                   long global_node_count) const {
        auto gid_range_for_rank =  Parfait::LinearPartitioner::getRangeForWorker(mp.Rank(),global_node_count,mp.NumberOfProcesses());
        std::vector<int> resident_iblank(gid_range_for_rank.end-gid_range_for_rank.start);
        auto status = mp.NonBlockingRecv(resident_iblank,0);
        if(0 == mp.Rank()) {
            auto& global_iblank = global_iblank_on_root;
            for (int rank = 0; rank < mp.NumberOfProcesses(); rank++) {
                auto range =  Parfait::LinearPartitioner::getRangeForWorker(rank,global_node_count,mp.NumberOfProcesses());
                std::vector<int> iblank_for_rank(global_iblank.begin()+range.start,global_iblank.begin()+range.end);
                mp.Send(iblank_for_rank,rank);
            }
        }
        status.wait();
        return resident_iblank;
    }
};

CREATE_INF_SUBCOMMAND(PlotDciCommand)
