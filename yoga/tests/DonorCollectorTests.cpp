#include <ZMQMessager.h>
#include <RingAssertions.h>
#include "DonorCollector.h"
#include "PortMapper.h"

using namespace YOGA;

TEST_CASE("process stream of receptors") {
    CandidateDonor candidateDonor;
    candidateDonor.component = 0;
    candidateDonor.distance = 0.5;

    Receptor receptor;
    receptor.owner = 0;
    receptor.globalId = 95;
    receptor.candidateDonors.push_back(candidateDonor);

    std::vector<Receptor> receptors;
    receptors.push_back(receptor);
    receptors.push_back(receptor);

    SECTION("test with single update") {
        MessagePasser::Message msg;
        msg.pack(DonorCollector::Receptors);
        Receptor::pack(msg, receptors);
        DonorCollector collector;
        collector.doWork(msg);
        REQUIRE(2 == collector.receptors.size());
    }

    SECTION("test with two updates") {
        MessagePasser::Message msg;
        msg.pack(DonorCollector::Receptors);
        Receptor::pack(msg, receptors);
        DonorCollector collector;
        collector.doWork(msg);
        MessagePasser::Message another_msg;
        another_msg.pack(DonorCollector::Receptors);
        Receptor::pack(another_msg, receptors);
        collector.doWork(another_msg);
        REQUIRE(4 == collector.receptors.size());
    }

}
