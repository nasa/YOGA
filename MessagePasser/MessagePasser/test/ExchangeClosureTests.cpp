#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>

TEST_CASE("Pack big stuff with closures into exchanges"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::map<int, std::map<long,std::set<long>>> big_old_map;

    big_old_map[0][999] = {89,99,193,mp.Rank()};

    auto packer = [](MessagePasser::Message& msg, const std::map<long,std::set<long>>& stencils){
       msg.pack(stencils);
    };

    auto unpacker = [](MessagePasser::Message& msg,std::map<long,std::set<long>>& stencils){
        msg.unpack(stencils);
    };


    auto stuff = mp.Exchange(big_old_map,packer,unpacker);

    if(0 == mp.Rank()){
        REQUIRE(mp.NumberOfProcesses() == int(stuff.size()));
        for(int rank=0;rank<mp.NumberOfProcesses();rank++) {
            REQUIRE(1 == stuff[rank].size());
            REQUIRE(4 == stuff[rank][999].size());
            REQUIRE(stuff[rank][999].count(rank) == 1);
        }
    }
}

TEST_CASE("Exchange messages"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::map<int, std::map<long,std::set<long>>> big_old_map;
    std::map<int, MessagePasser::Message> big_old_messages;

    big_old_map[0][999] = {89,99,193,mp.Rank()};

    for(auto& item:big_old_map){
        int key = item.first;
        big_old_messages[key].pack(item.second);
    }
    auto packed_messages = mp.Exchange(big_old_messages);


    std::map<int, std::map<long,std::set<long>>> stuff;
    for(auto& pair:packed_messages){
        pair.second.unpack(stuff[pair.first]);
    }

    if(0 == mp.Rank()){
        REQUIRE(mp.NumberOfProcesses() == int(stuff.size()));
        for(int rank=0;rank<mp.NumberOfProcesses();rank++) {
            REQUIRE(1 == stuff[rank].size());
            REQUIRE(4 == stuff[rank][999].size());
            REQUIRE(stuff[rank][999].count(rank) == 1);
        }
    }
}