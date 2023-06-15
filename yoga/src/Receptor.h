#pragma once
#include <MessagePasser/MessagePasser.h>
#include <algorithm>
namespace YOGA {

struct CandidateDonor {
    enum CellType {Tet,Pyramid,Prism,Hex};
    CandidateDonor() = default;
    CandidateDonor(int componentId, int cell, int owner, double dist,CellType type)
        : component(componentId), cellId(cell), cellOwner(owner), isValid(1), distance(dist), cell_type(type) {}

    int component;
    int cellId;
    int cellOwner;
    int isValid;
    double distance;
    int cell_type;
};

class Receptor {
  public:
    long globalId;
    int owner;
    double distance;
    std::vector<CandidateDonor> candidateDonors;
    std::vector<long> nbr_gids;

    static void pack(MessagePasser::Message& msg, const Receptor& r) {
        msg.pack(r.globalId);
        msg.pack(r.owner);
        msg.pack(r.distance);
        msg.pack(r.candidateDonors);
        msg.pack(r.nbr_gids);
    }

    static void pack(MessagePasser::Message& msg, const std::vector<Receptor>& receptors) {
        msg.pack(int(receptors.size()));
        for (auto& r : receptors) Receptor::pack(msg, r);
    }

    static void unpack(MessagePasser::Message& msg, Receptor& r) {
        msg.unpack(r.globalId);
        msg.unpack(r.owner);
        msg.unpack(r.distance);
        msg.unpack(r.candidateDonors);
        msg.unpack(r.nbr_gids);
    }

    static void unpack(MessagePasser::Message& msg, std::vector<Receptor>& receptors) {
        int n = 0;
        msg.unpack(n);
        for (int i = 0; i < n; i++) {
            Receptor r;
            Receptor::unpack(msg, r);
            receptors.emplace_back(r);
        }
    }
};

class ReceptorCollection{
  public:
    ReceptorCollection() = default;
    size_t size() const {return gids.size();}
    void insert(const Receptor& r){
        gids.push_back(r.globalId);
        owners.push_back(r.owner);
        distance.push_back(r.distance);
        int idx = getIndex();
        index_of_first_donor.push_back(idx);
        donor_counts.push_back(r.candidateDonors.size());
        for(auto& d:r.candidateDonors){
            donor_cell_ids.push_back(d.cellId);
            donor_cell_type.push_back(d.cell_type);
            donor_component_ids.push_back(d.component);
            donor_owning_ranks.push_back(d.cellOwner);
            donor_distance.push_back(d.distance);
        }
    }

    Receptor get(int i){
        Receptor r;
        r.globalId = gids[i];
        r.owner = owners[i];
        r.distance = distance[i];
        int first_donor = index_of_first_donor[i];
        for(int j=0;j<donor_counts[i];j++){
            CandidateDonor d;
            int index = first_donor+j;
            d.cellId = donor_cell_ids[index];
            d.cell_type = donor_cell_type[index];
            d.distance = donor_distance[index];
            d.component = donor_component_ids[index];
            d.cellOwner = donor_owning_ranks[index];
            r.candidateDonors.push_back(d);
        }
        return r;
    }

    void pack(MessagePasser::Message& msg){
        msg.pack(gids);
        msg.pack(owners);
        msg.pack(distance);
        msg.pack(donor_counts);
        msg.pack(index_of_first_donor);
        msg.pack(donor_owning_ranks);
        msg.pack(donor_cell_ids);
        msg.pack(donor_component_ids);
        msg.pack(donor_cell_type);
        msg.pack(donor_distance);
    }
    void unpack(MessagePasser::Message& msg){
        msg.unpack(gids);
        msg.unpack(owners);
        msg.unpack(distance);
        msg.unpack(donor_counts);
        msg.unpack(index_of_first_donor);
        msg.unpack(donor_owning_ranks);
        msg.unpack(donor_cell_ids);
        msg.unpack(donor_component_ids);
        msg.unpack(donor_cell_type);
        msg.unpack(donor_distance);
    }
  private:
    std::vector<long> gids;
    std::vector<int> owners;
    std::vector<double> distance;
    std::vector<int> donor_counts;
    std::vector<int> index_of_first_donor;
    std::vector<int> donor_owning_ranks;
    std::vector<int> donor_cell_ids;
    std::vector<int> donor_component_ids;
    std::vector<int> donor_cell_type;
    std::vector<double> donor_distance;

    int getIndex(){
        if(donor_counts.size() == 0){
            return 0;
        }
        return index_of_first_donor.back() + donor_counts.back();
    }
};

}
