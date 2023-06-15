
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space Administration. 
// No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
// 
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the 
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0. 
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed 
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.
#include <numeric>
#include <vector>
#include <array>
#include "../MessagePasser.h"
#include <RingAssertions.h>

TEST_CASE("Gather for integers"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0,value=mp.Rank();
    std::vector<int> vec;
    mp.Gather(value,vec,root);
    // check that they were communicated properly to the root
    if(mp.Rank() == root) {
        int nproc = mp.NumberOfProcesses();
        REQUIRE(nproc == (int)vec.size());
        for(int i=0;i<nproc;i++)
            REQUIRE(i == vec[i]);
    }
}

TEST_CASE("Gather floats"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0;
    float junk = 1.7e-12f;
    float value= junk + (float) mp.Rank();
    std::vector<float> vec;
    mp.Gather(value,vec,root);
    // check that they were communicated properly to the root
    if(mp.Rank() == root)
    {
        int nproc = mp.NumberOfProcesses();
        REQUIRE(nproc == (int)vec.size());
        for(int i=0;i<nproc;i++)
            REQUIRE((junk+(float)i) == vec[i]);
    }
}

TEST_CASE("gather vector of ints"){
    int root = 0;
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<int> send_vec;
    send_vec.push_back(mp.Rank());
    send_vec.push_back(mp.Rank()+1);
    std::vector<int> recv_vec;
    mp.Gather(send_vec,2,recv_vec,root);

    if(mp.Rank() == root) {
        int nproc = mp.NumberOfProcesses();
        REQUIRE((2*nproc) == (int)recv_vec.size());
        for(int i=0;i<nproc;i++) {
            REQUIRE(i == recv_vec[2*i]);
            REQUIRE((i+1) == recv_vec[2*i+1]);
        }
    }
}

TEST_CASE("Gather with actual return values"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<int> send_vec = {mp.Rank(), mp.Rank()};

    std::vector<std::vector<int>> from_everyone = mp.Gather(send_vec);
    REQUIRE(int(from_everyone.size()) == mp.NumberOfProcesses());
    for(auto& from : from_everyone){
        REQUIRE(from.size() == 2);
    }
    for(int r = 0; r < mp.NumberOfProcesses(); r++){
        for(int i = 0; i < 2; i++){
            auto from = from_everyone[r][i];
            REQUIRE(from == r);
        }
    }
}

TEST_CASE("integer scalars"){
    MessagePasser mp(MPI_COMM_WORLD);
    int value=mp.Rank();
    std::vector<int> vec;
    mp.Gather(value,vec);
    int nproc = mp.NumberOfProcesses();
    REQUIRE(nproc == (int)vec.size());
    for(int i=0;i<nproc;i++)
        REQUIRE(i == vec[i]);
}

TEST_CASE("float scalars"){
    MessagePasser mp(MPI_COMM_WORLD);

    double junk = 1.7e-13;
    double value=junk + (double)mp.Rank();
    std::vector<double> vec;
    mp.Gather(value,vec);
    int nproc = mp.NumberOfProcesses();
    REQUIRE(nproc == (int)vec.size());
    for(int i=0;i<nproc;i++)
        REQUIRE((junk+(double)i) == vec[i]);
}

TEST_CASE("TestWithIntegers"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<int> send_vec;
    std::vector<int> recv_vec;
    std::vector<int> map;
    int num = 0;
    // even and odd procs have size 2 & 3 vectors respectively
    if(mp.Rank() % 2 == 0)
        num = 2;
    else
        num = 3;
    for(int i=0;i<num;i++)
        send_vec.push_back(mp.Rank()+i);
    mp.Gather(send_vec,recv_vec,map);
    REQUIRE((mp.NumberOfProcesses()+1) == (int)map.size());
    REQUIRE(map.back() == (int)recv_vec.size());
    for(int i=0;i< mp.NumberOfProcesses();i++){
        int counter = 0;
        for(int j=map[i];j<map[i+1];j++){
            REQUIRE((i+counter) == recv_vec[j]);
            ++counter;
        }
    }
}

TEST_CASE("TestWithNoMapPassedFromRoot"){
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<int> send_vec;
    std::vector<int> recv_vec;
    std::vector<int> map(mp.NumberOfProcesses()+1);
    int num = 0;
    // even and odd procs have size 2 & 3 vectors respectively
    if(mp.Rank() % 2 == 0)
        num = 2;
    else
        num = 3;
    for(int i=0;i<num;i++)
        send_vec.push_back(mp.Rank()+i);
    mp.Gather(send_vec,recv_vec);

    for(int i=1;i<mp.NumberOfProcesses()+1;i++){
        num = (i-1)%2==0 ? 2 : 3;
        map[i] = map[i-1] + num;
    }
    REQUIRE((mp.NumberOfProcesses()+1) == (int)map.size());
    REQUIRE(map.back() == (int)recv_vec.size());
    for(int i=0;i<mp.NumberOfProcesses();i++) {
        int counter = 0;
        for(int j=map[i];j<map[i+1];j++) {
            REQUIRE((i+counter) == recv_vec[j]);
            ++counter;
        }
    }
}

TEST_CASE("TestWithVectorOfVectors"){
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector <int> send_vec;
    std::vector <std::vector<int>> result;
    std::vector <int> map(mp.NumberOfProcesses() + 1);
    int num = 0;
    // even and odd procs have size 2 & 3 vectors respectively
    if (mp.Rank() % 2 == 0)
        num = 2;
    else
        num = 3;
    for (int i = 0; i < num; i++)
        send_vec.push_back(mp.Rank() + i);
    mp.Gather(send_vec, result);
    for(int i=0;i<mp.NumberOfProcesses();i++){
        if(i % 2 == 0) {
            REQUIRE(2 == result[i].size());
            for(int j=0;j<2;j++)
                REQUIRE((i+j) == result[i][j]);
        }
        else{
            REQUIRE(3 == result[i].size());
            for(int j=0;j<3;j++)
                REQUIRE((i+j) == result[i][j]);
        }

    }
}

TEST_CASE("Test Gather for integers"){
    MessagePasser mp(MPI_COMM_WORLD);
    int root = 0;
    std::vector<int> send_vec;
    std::vector<int> recv_vec;
    std::vector<int> map;
    int num = 0;
    // even and odd procs have size 2 & 3 vectors respectively
    if(mp.Rank() % 2 == 0)
        num = 2;
    else
        num = 3;
    for(int i=0;i<num;i++)
        send_vec.push_back(mp.Rank()+i);
    mp.Gather(send_vec,recv_vec,map,root);
    if(mp.Rank() == root) {
        REQUIRE((mp.NumberOfProcesses()+1) == (int)map.size());
        REQUIRE(map.back() == (int)recv_vec.size());
        for(int i = 0; i < mp.NumberOfProcesses(); i++) {
            int counter = 0;
            for(int j = map[i]; j < map[i+1]; j++) {
                REQUIRE((i+counter) == recv_vec[j]);
                ++counter;
            }
        }
    }

}

TEST_CASE("TestWithIntegersNoMapPassedIn"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0;
    std::vector<int> send_vec;
    std::vector<int> recv_vec;
    int num = 0;
    // even and odd procs have size 2 & 3 vectors respectively
    if(mp.Rank() % 2 == 0)
        num = 2;
    else
        num = 3;
    for(int i=0;i<num;i++)
        send_vec.push_back(mp.Rank()+i);
    mp.Gather(send_vec,recv_vec,root);
    // just make sure it gets called, since it is just a delegation
}

TEST_CASE("TestWithFloats"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0;
    std::vector<float> send_vec;
    std::vector<float> recv_vec;
    float junk = 1.7e-5f;
    std::vector<int> map;
    int num = 0;
    // even and odd procs have size 2 & 3 vectors respectively
    if(mp.Rank() % 2 == 0)
        num = 2;
    else
        num = 3;
    for(int i=0;i<num;i++)
        send_vec.push_back(junk+(float)(mp.Rank()+i));
    mp.Gather(send_vec,recv_vec,map,root);
    if(mp.Rank() == root) {
        REQUIRE((mp.NumberOfProcesses()+1) == (int)map.size());
        REQUIRE(map.back() == (int)recv_vec.size());
        for(int i=0;i<mp.NumberOfProcesses();i++) {
            int counter = 0;
            for(int j=map[i];j<map[i+1];j++) {
                REQUIRE((junk+(float)(i+counter)) == Approx(recv_vec[j]));
                ++counter;
            }
        }
    }
}

TEST_CASE("TestWithDoubles"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0;
    std::vector<double> send_vec;
    std::vector<double> recv_vec;
    double junk = 1.7e-13;
    std::vector<int> map;
    int num = 0;
// even and odd procs have size 2 & 3 vectors respectively
    if(mp.Rank() % 2 == 0)
        num = 2;
    else
        num = 3;
    for(int i=0;i<num;i++)
        send_vec.push_back(junk+(double)(mp.Rank()+i));
    mp.Gather(send_vec,recv_vec,map,root);
    if(mp.Rank() == root) {
        REQUIRE((mp.NumberOfProcesses()+1) == (int)map.size());
        REQUIRE(map.back() == (int)recv_vec.size());
        for(int i=0;i<mp.NumberOfProcesses();i++) {
            int counter = 0;
            for(int j=map[i];j<map[i+1];j++) {
                REQUIRE((junk+(double)(i+counter)) == Approx(recv_vec[j]));
                ++counter;
            }
        }
    }
}

TEST_CASE("TestWhenAllVectorsAreEmpty"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0;
    std::vector<int> send_vec;
    std::vector<int> recv_vec;
    std::vector<int> map;
    mp.Gather(send_vec,recv_vec,map,root);
    if(mp.Rank() == root) {
        REQUIRE((mp.NumberOfProcesses()+1) == (int)map.size());
        REQUIRE(0 == (int)recv_vec.size());
        REQUIRE(0 == map.back());
    }
}

TEST_CASE("TestGatheringAsVectorOfVectors"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0;
    std::vector<double> send_vec;
    std::vector<std::vector<double>> result;
    double junk = 1.7e-13;
    std::vector<int> map;
    int num = 0;
// even and odd procs have size 2 & 3 vectors respectively
    if(mp.Rank() % 2 == 0)
        num = 2;
    else
        num = 3;
    for(int i=0;i<num;i++)
        send_vec.push_back(junk+(double)(mp.Rank()+i));
    mp.Gather(send_vec, result,root);
    if(mp.Rank() == root) {
        REQUIRE(mp.NumberOfProcesses() == (int) result.size());
        for(int i=0;i<mp.NumberOfProcesses();i++) {
            int counter = 0;
            REQUIRE((i%2==0?2:3) == result[i].size());
            for(unsigned int j=0;j< result[i].size();j++) {
                REQUIRE((junk+(double)(i+counter)) == Approx(result[i][j]));
                ++counter;
            }
        }
    }
}

TEST_CASE("return gathers with root"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::vector<long> some_vector(10);
    std::iota(std::begin(some_vector), std::end(some_vector), 10*mp.Rank());

    auto all_vectors = mp.Gather(some_vector, 0);
    if(mp.Rank() == 0) {
        REQUIRE(int(all_vectors.size()) == mp.NumberOfProcesses());
        for (int r = 0; r < mp.NumberOfProcesses(); r++) {
            REQUIRE(all_vectors[r].size() == 10);
        }
    } else {
        REQUIRE(all_vectors.size() == 0);
    }
}

TEST_CASE("Gather by ids and throw if object length doesn't match ids"){
    std::vector<std::array<double, 3>> points;
    std::vector<long> global_ids = {-99};
    MessagePasser mp(MPI_COMM_WORLD);

    for(int i = 0; i < 10; i++){
        double x = mp.Rank()*10 + i;
        points.push_back({x,x,x});
        global_ids.push_back(10*mp.Rank() + i);
    }

    int root = 0;
    REQUIRE_THROWS(mp.GatherByIds(points, global_ids, root));
}

TEST_CASE("Gather and sort by ids"){
    std::vector<std::array<double, 3>> points;
    std::vector<long> global_ids;
    MessagePasser mp(MPI_COMM_WORLD);

    for(int i = 0; i < 10; i++){
        double x = mp.Rank()*10 + i;
        points.push_back({x,x,x});
        global_ids.push_back(10*mp.Rank() + i);
    }

    SECTION("gather to one rank") {
        int root = 0;
        points = mp.GatherByIds(points, global_ids, root);
        if (mp.Rank() == root) {
            REQUIRE(int(points.size()) == mp.NumberOfProcesses() * 10);
            for (int i = 0; i < 10 * mp.NumberOfProcesses(); i++) {
                auto x = double(i);
                REQUIRE(points[i][0] == x);
                REQUIRE(points[i][1] == x);
                REQUIRE(points[i][2] == x);
            }
        } else {
            REQUIRE(points.size() == 0);
        }
    }

    SECTION("gather to all rank") {
        points = mp.GatherByIds(points, global_ids);
        REQUIRE(int(points.size()) == mp.NumberOfProcesses() * 10);
        for (int i = 0; i < 10 * mp.NumberOfProcesses(); i++) {
            auto x = double(i);
            REQUIRE(points[i][0] == x);
            REQUIRE(points[i][1] == x);
            REQUIRE(points[i][2] == x);
        }
    }
}

TEST_CASE("Gather and sort by ids with stride"){
    std::vector<long> tets;
    std::vector<long> tet_gids;
    MessagePasser mp(MPI_COMM_WORLD);
    for(int i = 0; i < 10; i++){
        long tet_id = mp.Rank()*10+i;
        tet_gids.push_back(tet_id);
        for(int j = 0; j < 4; j++){
            tets.push_back(mp.Rank()*10+j);
        }
    }

    SECTION("To one rank") {
        int root = 0;
        std::vector<long> all_tets = mp.GatherAndSort(tets, 4, tet_gids, root);
        if (mp.Rank() == root) {
            REQUIRE(int(all_tets.size()) == 4 * mp.NumberOfProcesses() * 10);
        } else {
            REQUIRE(int(all_tets.size()) == 0);
        }
    }

    SECTION("To all ranks"){
        std::vector<long> all_tets = mp.GatherAndSort(tets, 4, tet_gids);
        REQUIRE(int(all_tets.size()) == 4 * mp.NumberOfProcesses() * 10);
    }
}

TEST_CASE("Gather by range"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::map<long, int> my_data = {{mp.Rank(), mp.Rank()}};

    SECTION("All gather") {
        auto collapsed_data = mp.GatherByIds(my_data);
        REQUIRE(int(collapsed_data.size()) == mp.NumberOfProcesses());
        for(int i = 0; i < mp.NumberOfProcesses(); i++){
            REQUIRE(collapsed_data[i] == i);
        }
    }
    SECTION("gather to root"){
        auto collapsed_data = mp.GatherByIds(my_data, 0);
        if(mp.Rank() == 0) {
            REQUIRE(int(collapsed_data.size()) == mp.NumberOfProcesses());
            for (int i = 0; i < mp.NumberOfProcesses(); i++) {
                REQUIRE(collapsed_data[i] == i);
            }
        }
        else
            REQUIRE(collapsed_data.empty());
    }
}

TEST_CASE("gather map with range"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::map<long, int> my_data = {{mp.Rank(), mp.Rank()}};
    long start = 0;
    long end = 1;

    SECTION("Collect on all ranks by default"){
        auto collapsed_data = mp.GatherByIds(my_data,start,end);
        REQUIRE(1 == collapsed_data.size());
        REQUIRE(0 == collapsed_data.at(0));
    }

    SECTION("only gather on root"){
        int root_id = 0;
        auto collapsed_data = mp.GatherByOrdinalRange(my_data, start, end, root_id);
        if(root_id == mp.Rank()){
            REQUIRE(1 == collapsed_data.size());
            REQUIRE(0 == collapsed_data.at(0));
        }
        else{
            REQUIRE(0 == collapsed_data.size());
        }
    }

    SECTION("Make sure sending extra stuff is ok"){
        my_data[mp.Rank()+mp.NumberOfProcesses()] = 0;
        REQUIRE(2 == my_data.size());
        auto collapsed_data = mp.GatherByOrdinalRange(my_data, start, end, 0);
        if(0 == mp.Rank()) {
            REQUIRE(1 == collapsed_data.size());
            REQUIRE(0 == collapsed_data.at(0));
        }
        else{
            REQUIRE(0 == collapsed_data.size());
        }
    }
}

TEST_CASE("Gather just a type T"){
    MessagePasser mp(MPI_COMM_WORLD);
    int a = mp.Rank();

    auto all_a = mp.Gather(a);
    REQUIRE(int(all_a.size()) == mp.NumberOfProcesses());
    REQUIRE(int(all_a.size()) == mp.NumberOfProcesses());
    for(int i = 0; i < int(all_a.size()); i++){
        REQUIRE(all_a[i] == i);
    }
}

TEST_CASE("Gather range by implied id"){
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<int> some_stuff = {mp.Rank()};
    long start = 0;
    long end = mp.NumberOfProcesses();
    auto some_gathered_stuff = mp.Gather(some_stuff, start, end);
    REQUIRE(int(some_gathered_stuff.size()) == end-start);
}

TEST_CASE("Gather range by implied id to root"){
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<int> some_stuff = {mp.Rank()};
    long start = 0;
    long end = mp.NumberOfProcesses();
    int root = 0;
    auto some_gathered_stuff = mp.Gather(some_stuff, start, end, root);
    if(mp.Rank() == root)
        REQUIRE(int(some_gathered_stuff.size()) == end-start);
    else
        REQUIRE(0 == some_gathered_stuff.size());
}
