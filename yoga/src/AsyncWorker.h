#pragma once
#include "DonorDistributor.h"
#include "GridFetcher.h"
#include "NanoFlannDistanceCalculator.h"
#include "ScalableHoleMap.h"
#include "WorkVoxelBuilder.h"
#include "ZMQPostMan.h"
#include "OverlapDetector.h"
#include "WorkVoxel.h"
#include "VoxelDonorFinder.h"
namespace YOGA {
class AsyncWorker {
  public:
    AsyncWorker(int rank,int nranks,
                MessagePasser mp,
                ZMQPostMan& post_man,
                ZMQPostMan::MailBox& mailbox,
                const std::vector<std::vector<Parfait::Point<double>>>& surfaces,
                std::function<bool(double*, int, double*)> is_in_cell,
                const OverlapDetector& overlap_detector,
                const std::vector<bool>& initial_work_unit_mask,
                const std::vector<Parfait::Extent<double>>& initial_work_units,
                const std::vector<int>& host_ranks,
                GridFetcher& grid_fetcher)
        :
          mp(mp),
          intitial_work_unit_mask(initial_work_unit_mask),
          initial_work_units(initial_work_units),
          rank_of_my_load_balancer(chooseLoadBalancer(rank,host_ranks)),
          grid_fetcher(grid_fetcher),
          my_rank(rank),
          nranks(nranks),
          post_man(post_man),
          mailbox(mailbox),
          distance_calculator(surfaces),
          is_in_cell(is_in_cell),
          overlap_detector(overlap_detector){}

    void workUntilFinished() {
        auto initial_fragments = sendFragmentsBasedOnInitialWorkUnits();
        if(intitial_work_unit_mask[my_rank]){
            processFirstWorkUnit(initial_work_units[my_rank],initial_fragments);
        }
        initial_fragments.clear();
        if(mp.Rank()==0 and mp.NumberOfProcesses()>100){
            // don't do work, since I'm expecting lots of requests
            // and want to minimize latency
        }
        else {
            Tracer::begin("request work unit");
            MessagePasser::Message msg;
            msg.pack(my_rank);
            post_man.push(rank_of_my_load_balancer, WorkRequest, std::move(msg));
            Tracer::end("request work unit");
            Tracer::begin("wait");
            auto next_work_unit_ptr = mailbox.wait(MessageTypes::WorkExtentBox);
            auto next_work_unit = *next_work_unit_ptr;
            Tracer::end("wait");
            int n_remaining = 0;
            next_work_unit.unpack(n_remaining);
            while (n_remaining > 0) {
                Parfait::Extent<double> e;
                next_work_unit.unpack(e);
                processWorkUnit(e);
                Tracer::begin("request work unit");
                MessagePasser::Message work_request;
                work_request.pack(my_rank);
                post_man.push(rank_of_my_load_balancer, MessageTypes::WorkRequest, std::move(work_request));
                Tracer::end("request work unit");
                Tracer::begin("wait");
                next_work_unit_ptr = mailbox.wait(MessageTypes::WorkExtentBox);
                next_work_unit = *next_work_unit_ptr;
                Tracer::end("wait");
                next_work_unit.unpack(n_remaining);
            }
        }

        waitForDciReceipts();
        sendCompletionNotifications();
        sleepUntilOtherProcessesNotifyMe();
        extractDciUpdates();
    }

    void waitForDciReceipts(){
        for(int i=0;i<dci_send_count;i++){
            mailbox.wait(DciReceipt);
        }
    }

    std::vector<Receptor> receptors;
    std::vector<int> getVoxelSizes(){
        return nodes_per_work_unit;
    }

  private:
    MessagePasser mp;
    const std::vector<bool>& intitial_work_unit_mask;
    const std::vector<Parfait::Extent<double>>& initial_work_units;
    int rank_of_my_load_balancer;
    GridFetcher& grid_fetcher;
    const int my_rank;
    const int nranks;
    ZMQPostMan& post_man;
    ZMQPostMan::MailBox& mailbox;
    NanoFlannDistanceCalculator distance_calculator;
    std::function<bool(double*, int, double*)> is_in_cell;
    const OverlapDetector& overlap_detector;
    int dci_send_count = 0;
    std::vector<int> nodes_per_work_unit;

    std::map<int,MessagePasser::Message> sendFragmentsBasedOnInitialWorkUnits(){
        Tracer::begin("prep initial fragments");
        std::map<int,MessagePasser::Message> fragments_for_ranks;
        for(int i=0;i<nranks;i++){
            if(intitial_work_unit_mask[i]) {
                auto& e = initial_work_units[i];
                if(overlap_detector.doesOverlap(e,my_rank)){
                    MessagePasser::Message msg;
                    msg.pack(e);
                    fragments_for_ranks[i] = grid_fetcher.doWork(msg);
                    //post_man.push(i,GridFragment,std::move(reply));
                }
            }
        }
        Tracer::end("prep initial fragments");
        return mp.Exchange(fragments_for_ranks);
    }

    void processFirstWorkUnit(const Parfait::Extent<double>& e,std::map<int,MessagePasser::Message>& fragment_msgs){
        Tracer::begin("process work unit");
        WorkVoxel voxel(e, is_in_cell);
        for(auto& pair:fragment_msgs){
            auto& msg = pair.second;
            extractFragmentsFromMsg(voxel,msg);
        }

        nodes_per_work_unit.push_back(voxel.nodes.size());
        calcAndSetDistanceToNodes(voxel);
        auto n2n = createNodeNeighbors(voxel);
        auto candidate_receptors = VoxelDonorFinder::getCandidateReceptors(voxel, e, n2n);
        auto stuff_for_owners = DonorDistributor::mapReceptorsToOwners(candidate_receptors);
        sendDciUpdates(stuff_for_owners);
        Tracer::end("process work unit");

    }

    void processWorkUnit(const Parfait::Extent<double>& e) {
        Tracer::begin("process work unit");
        int n = requestFragments(e);
        WorkVoxel voxel(e, is_in_cell);
        waitOnFragments(n,voxel);

        nodes_per_work_unit.push_back(voxel.nodes.size());
        calcAndSetDistanceToNodes(voxel);
        auto n2n = createNodeNeighbors(voxel);
        auto candidate_receptors = VoxelDonorFinder::getCandidateReceptors(voxel, e, n2n);
        auto stuff_for_owners = DonorDistributor::mapReceptorsToOwners(candidate_receptors);
        sendDciUpdates(stuff_for_owners);
        Tracer::end("process work unit");
    }

    void sendDciUpdates(const std::map<int, std::vector<Receptor>>& stuff_for_owners) {
        Tracer::begin("send dci updates");
        for (auto& stuff : stuff_for_owners) {
            MessagePasser::Message msg;
            msg.pack(my_rank);
            int owning_rank = stuff.first;
            auto& r = stuff.second;
            Receptor::pack(msg, r);
            dci_send_count++;
            post_man.push(owning_rank, MessageTypes::DciUpdate, std::move(msg));
        }
        Tracer::end("send dci updates");
    }

    void sendCompletionNotifications() {
        post_man.push(0,WorkUnitsComplete,MessagePasser::Message());
    }

    void fillNeighbors(std::vector<std::set<int>>& n2n,const int* cell,int cell_size,
        const std::vector<bool>& is_outside_voxel){
        for(int i=0;i<cell_size;i++){
            for(int j=i+1;j<cell_size;j++){
                int left = cell[i];
                int right = cell[j];
                if(not is_outside_voxel[left]) n2n[left].insert(right);
                if(not is_outside_voxel[right]) n2n[right].insert(left);
            }
        }
    }

    std::vector<bool> flagNodesOutsideVoxel(const WorkVoxel& w) {
        std::vector<bool> is_outside(w.nodes.size(), false);
        for (int i = 0; i < int(w.nodes.size()); ++i)
            if (not w.extent.intersects(w.nodes[i].xyz)) is_outside[i] = true;
        return is_outside;
    }

    std::vector<std::set<int>> createNodeNeighbors(const WorkVoxel& voxel){
        Tracer::begin("n2n");
        auto is_outside = flagNodesOutsideVoxel(voxel);
        std::vector<std::set<int>> n2n(voxel.nodes.size());
        for(int i=0;i<int(voxel.tets.size());i++){
            auto& cell = voxel.tets[i];
            fillNeighbors(n2n,cell.nodeIds.data(),4,is_outside);
        }
        for(int i=0;i<int(voxel.pyramids.size());i++){
            auto& cell = voxel.pyramids[i];
            fillNeighbors(n2n,cell.nodeIds.data(),5,is_outside);
        }
        for(int i=0;i<int(voxel.prisms.size());i++){
            auto& cell = voxel.prisms[i];
            fillNeighbors(n2n,cell.nodeIds.data(),6,is_outside);
        }
        for(int i=0;i<int(voxel.hexs.size());i++){
            auto& cell = voxel.hexs[i];
            fillNeighbors(n2n,cell.nodeIds.data(),8,is_outside);
        }
        Tracer::end("n2n");
        return n2n;
    }

    void calcAndSetDistanceToNodes(WorkVoxel& voxel){
        Tracer::begin("distance");
        int n = voxel.nodes.size();
        std::vector<Parfait::Point<double>> points(n);
        std::vector<int> grid_ids(n);
        for(int i=0;i<int(voxel.nodes.size());i++){
            auto& node = voxel.nodes[i];
            points[i] = node.xyz;
            grid_ids[i] = node.associatedComponentId;
        }
        auto d = distance_calculator.calculateDistances(points,grid_ids);
        for(int i=0;i<int(points.size());i++){
            auto& node = voxel.nodes[i];
            node.distanceToWall = d[i];
        }
        Tracer::end("distance");
    }

    void waitOnFragments(int n, WorkVoxel& voxel) const {
        for (int i = 0; i < n; i++) {
            Tracer::begin("wait on fragment");
            auto msg = mailbox.wait(GridFragment);
            Tracer::end("wait on fragment");
            extractFragmentsFromMsg(voxel, *msg);
        }
    }

    void extractFragmentsFromMsg(WorkVoxel& voxel, MessagePasser::Message& msg) const {
        int nfragments = 0;
        msg.unpack(nfragments);
        Tracer::begin("unpack fragments");
        for (int j = 0; j < nfragments; j++) {
            extractFragment(voxel, msg);
        }
        Tracer::end("unpack fragments");
    }

    void extractFragment(WorkVoxel& voxel, MessagePasser::Message& msg) const {
        VoxelFragment fragment;
        msg.unpack(fragment.transferNodes);
        msg.unpack(fragment.transferTets);
        msg.unpack(fragment.transferPyramids);
        msg.unpack(fragment.transferPrisms);
        msg.unpack(fragment.transferHexs);
        std::vector<int> new_local_ids;
        voxel.addNodes(fragment.transferNodes,new_local_ids);
        voxel.addTets(fragment.transferTets, fragment.transferNodes, new_local_ids);
        voxel.addPyramids(fragment.transferPyramids, fragment.transferNodes, new_local_ids);
        voxel.addPrisms(fragment.transferPrisms, fragment.transferNodes, new_local_ids);
        voxel.addHexs(fragment.transferHexs, fragment.transferNodes, new_local_ids);
    }

    int requestFragments(const Parfait::Extent<double>& e) const {
                auto grid_server_ids = overlap_detector.getOverlappingRanks(e);
                for (int id : grid_server_ids) {
                        Tracer::begin("request fragment from " + std::to_string(id));
                        MessagePasser::Message msg;
                        msg.pack(my_rank);
                        msg.pack(e);
                        post_man.push(id, GridRequest, std::move(msg));
                        Tracer::end("request fragment from " + std::to_string(id));
                    }
                return grid_server_ids.size();
    }

    void sleepUntilOtherProcessesNotifyMe() {
        Tracer::begin("wait for other ranks to finish");
        int minions_per_captain = 20;
        if(my_rank == 0) {
            for (int i = 0; i < nranks; i++)
                mailbox.wait(MessageTypes::WorkUnitsComplete);
            for(int i=minions_per_captain; i < nranks; i+=minions_per_captain){
                int captain_id = i;
                post_man.push(captain_id,WorkUnitsComplete,MessagePasser::Message());
            }
            int n = std::min(minions_per_captain,nranks);
            for(int i=1;i<n;i++) {
                post_man.push(i, WorkUnitsComplete, MessagePasser::Message());
            }
        }
        else if(my_rank % minions_per_captain == 0){
            mailbox.wait(MessageTypes::WorkUnitsComplete);
            for(int i=0;i<minions_per_captain;i++){
                int minion_id = my_rank + i;
                if(minion_id >= nranks)
                    break;
                post_man.push(minion_id,WorkUnitsComplete,MessagePasser::Message());
            }
        }
        else{
            mailbox.wait(MessageTypes::WorkUnitsComplete);
        }
        Tracer::end("wait for other ranks to finish");
    }

    void extractDciUpdates() {
        Tracer::begin("extract dci updates");
        receptors.clear();
        while (mailbox.isThereAMessage(MessageTypes::DciUpdate)) {
            auto msg_ptr = mailbox.wait(MessageTypes::DciUpdate);
            auto msg = *msg_ptr;
            std::vector<Receptor> newReceptors;
            Receptor::unpack(msg, newReceptors);
            receptors.insert(receptors.end(), newReceptors.begin(), newReceptors.end());
        }
        Tracer::end("extract dci updates");
    }

    int chooseLoadBalancer(int my_rank,const std::vector<int>& host_ranks){
        int rank_of_load_balancer = host_ranks.front();
        for(int r:host_ranks){
            if(r > my_rank) break;
            rank_of_load_balancer = r;
        }
        return rank_of_load_balancer;
    }
};
}
