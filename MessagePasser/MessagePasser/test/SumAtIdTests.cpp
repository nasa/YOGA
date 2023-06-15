#include <RingAssertions.h>
#include <map>
#include <MessagePasser/MessagePasser.h>

TEST_CASE("Sum at id"){
    auto mp = MessagePasser(MPI_COMM_WORLD);
    typedef long GlobalId;
    typedef double TypeWithPlusEqualsOperator;
    std::map<GlobalId, TypeWithPlusEqualsOperator > data_to_sum;
    data_to_sum[100] = 5;
    data_to_sum[101] = 6;
    data_to_sum[108] = 7;

    auto get_owner = [&](GlobalId global_id){
        // normally do something like
        // local_id = g2l.at(global_id);
        // return mesh.nodeOwner(local_id);
        // but we're just going to return 0 for simplicity
        return 0;
    };
    std::map<GlobalId, TypeWithPlusEqualsOperator > summed_data = mp.SumAtId(data_to_sum, get_owner);
    REQUIRE(summed_data.size() == 3);
    REQUIRE(summed_data.at(100) == 5*mp.NumberOfProcesses());
    REQUIRE(summed_data.at(101) == 6*mp.NumberOfProcesses());
    REQUIRE(summed_data.at(108) == 7*mp.NumberOfProcesses());
}
