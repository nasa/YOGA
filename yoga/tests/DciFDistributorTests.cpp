#include <RingAssertions.h>
#include <DcifDistributor.h>

using namespace YOGA;

Dcif::FlattenedDomainConnectivity TwoReceptorDcifFixture(){
    int number_of_component_grids = 2;
    Dcif::FlattenedDomainConnectivity dcif(number_of_component_grids);
    dcif.setReceptorIds(std::vector<long>{99,105});
    dcif.setDonorCounts(std::vector<int>{8,4});
    dcif.setDonorIds(std::vector<long>{101,102,103,104,105,106,107,108,201,202,203,204});
    dcif.setDonorWeights({.2,.1,.2,.1,.1,.1,.1,.1,.25,.25,.25,.25});
    std::vector<int> statuses_for_all_nodes(300,1);
    statuses_for_all_nodes[99] = -1;
    statuses_for_all_nodes[105] = -1;
    dcif.setIBlank(statuses_for_all_nodes);
    return dcif;
}

TEST_CASE("extract individual receptors from global receptor data"){
    auto dcif = TwoReceptorDcifFixture();
    std::vector<long> my_gids {99,105};

    REQUIRE(2 == dcif.receptorCount());
    auto receptor = dcif.getReceptor(0);
    REQUIRE(99 == receptor.gid);
    REQUIRE(8 == receptor.donor_ids.size());
    for(long gid:receptor.donor_ids){
        REQUIRE(gid > 100);
        REQUIRE(gid < 109);
    }
    for(double weight:receptor.donor_weights){
        REQUIRE(weight > 0.0);
        REQUIRE(weight < 0.21);
    }
    REQUIRE(8 == receptor.donor_weights.size());

    receptor = dcif.getReceptor(1);
    REQUIRE(105 == receptor.gid);
    REQUIRE(4 == receptor.donor_ids.size());
    REQUIRE(4 == receptor.donor_weights.size());
    for(long gid:receptor.donor_ids){
        REQUIRE(gid > 200);
        REQUIRE(gid < 205);
    }
    for(double weight:receptor.donor_weights)
        REQUIRE(weight == .25);
}


TEST_CASE("Pack DcifReceptors into Messages"){
    auto dcif = TwoReceptorDcifFixture();
    MessagePasser::Message msg;
    msg.pack(dcif.receptorCount());
    for(int i=0;i<dcif.receptorCount();i++) {
        auto receptor = dcif.getReceptor(i);
        receptor.pack(msg);
    }

    auto unpacked_receptors = Dcif::unpackReceptors(msg);

    for(int i=0;i<dcif.receptorCount();i++) {
        auto original_receptor = dcif.getReceptor(i);
        auto& unpacked_receptor = unpacked_receptors[i];
        REQUIRE(unpacked_receptor.gid == original_receptor.gid);
        REQUIRE(unpacked_receptor.donor_ids == original_receptor.donor_ids);
        REQUIRE(unpacked_receptor.donor_weights == original_receptor.donor_weights);
    }
}


TEST_CASE("exchange dcif receptors"){
    MessagePasser mp(MPI_COMM_SELF);
    auto dcif = TwoReceptorDcifFixture();
    std::map<int,std::vector<long>> rank_to_requested_receptors;
    rank_to_requested_receptors[0] = dcif.getFringeIds();

    auto n = dcif.receptorCount();
    auto resident_receptors = getChunkOfReceptors(dcif,0,n);
    auto unpacked_receptors = exchangeReceptors(mp, resident_receptors, rank_to_requested_receptors);

    for(int i=0;i<dcif.receptorCount();i++) {
        auto original_receptor = dcif.getReceptor(i);
        auto& unpacked_receptor = unpacked_receptors[i];
        REQUIRE(unpacked_receptor.gid == original_receptor.gid);
        REQUIRE(unpacked_receptor.donor_ids == original_receptor.donor_ids);
        REQUIRE(unpacked_receptor.donor_weights == original_receptor.donor_weights);
    }
}

TEST_CASE("Extract chunks of DcifReceptors and map to ranks in linearly partitioned space"){
    auto dcif = TwoReceptorDcifFixture();

    long global_node_count = 300;
    int nranks = 3;
    SECTION("extract a range that only includes the first receptor") {
        auto receptors = getChunkOfReceptors(dcif, 0, 1);
        REQUIRE(1 == receptors.size());
        auto receptors_for_ranks = Dcif::mapReceptorsToRanks(receptors, global_node_count, nranks);
        REQUIRE(1 == receptors_for_ranks.size());
        REQUIRE(receptors_for_ranks.count(0) == 1);
    }

    SECTION("extract a range that includes both receptors") {
        auto receptors = getChunkOfReceptors(dcif, 0, 2);
        REQUIRE(2 == receptors.size());
        auto receptors_for_ranks = Dcif::mapReceptorsToRanks(receptors, global_node_count, nranks);
        REQUIRE(2 == receptors_for_ranks.size());
        REQUIRE(1 == receptors_for_ranks.count(0));
        REQUIRE(1 == receptors_for_ranks.count(1));
    }
}