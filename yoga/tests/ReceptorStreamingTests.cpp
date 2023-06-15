#include <RingAssertions.h>
#include "Receptor.h"

using namespace YOGA;

TEST_CASE("Stream receptors") {
    Receptor r1a;
    r1a.globalId = 0;
    r1a.owner = 1;
    r1a.candidateDonors.push_back(CandidateDonor(0, 9, 0, 0.5,CandidateDonor::Tet));

    MessagePasser::Message msg;

    SECTION("stream a single receptor, and check it") {
        Receptor::pack(msg, r1a);

        Receptor r1b;
        Receptor::unpack(msg, r1b);

        REQUIRE(r1a.owner == r1b.owner);
        REQUIRE(r1a.globalId == r1b.globalId);
    }

    SECTION("Stream 2 receptors and make sure two come out") {
        std::vector<Receptor> receptors;
        receptors.push_back(r1a);
        receptors.push_back(r1a);

        Receptor::pack(msg, receptors);

        std::vector<Receptor> receptors2;
        Receptor::unpack(msg, receptors2);
        REQUIRE(2 == receptors2.size());
    }
    SECTION("Stream a receptor collection") {
        ReceptorCollection collection_1,collection_2;
        collection_1.insert(r1a);
        collection_1.insert(r1a);

        collection_1.pack(msg);
        collection_2.unpack(msg);
        REQUIRE(collection_1.size() == collection_2.size());
        for(size_t i=0;i<collection_1.size();i++){
            auto a = collection_1.get(i);
            auto b = collection_2.get(i);
            REQUIRE(a.globalId == b.globalId);
            REQUIRE(a.owner == b.owner);
            REQUIRE(a.candidateDonors.size() == b.candidateDonors.size());
            for(size_t j=0;j<a.candidateDonors.size();j++){
                auto& d1 = a.candidateDonors[j];
                auto& d2 = b.candidateDonors[j];
                REQUIRE(d1.cellId == d2.cellId);
                REQUIRE(d1.component == d2.component);
                REQUIRE(d1.cellOwner == d2.cellOwner);
                REQUIRE(d1.cell_type == d2.cell_type);
            }
        }
    }

}

