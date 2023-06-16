#include <RingAssertions.h>
#include "PartVectorIO.h"
#include "FileWait.h"
#include <parfait/FileTools.h>
#include <thread>

using namespace YOGA;

TEST_CASE("collect part vector and dump to file"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::string filename = "parition.data";
    int max_seconds_to_wait_for_file = 5;

    if(mp.NumberOfProcesses() == 1) {
        std::vector<long> owned_global_node_ids {3,4,2,0,1};
        PartVectorIO exporter(mp,owned_global_node_ids);
        PartVectorIO importer(mp,owned_global_node_ids);
        exporter.exportPartVector(filename);
        waitForFileToExist(filename,max_seconds_to_wait_for_file);
        auto part = importer.importPartVector(filename);

        REQUIRE(owned_global_node_ids.size() == part.size());
        for(int rank:part){
            REQUIRE(0 == rank);
        }
    }
    else{
        std::vector<long> owned_global_node_ids {mp.Rank()};
        PartVectorIO exporter(mp,owned_global_node_ids);
        PartVectorIO importer(mp,owned_global_node_ids);
        exporter.exportPartVector(filename);
        waitForFileToExist(filename,max_seconds_to_wait_for_file);
        auto part = importer.importPartVector(filename);

        REQUIRE(owned_global_node_ids.size() == part.size());
        REQUIRE(mp.Rank() == part.front());
    }
}