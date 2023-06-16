
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
#include "../MessagePasser.h"
#include <RingAssertions.h>
#include <set>
#include <array>

TEST_CASE("Broadcast an integer"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0,val=0;
    if(mp.Rank() == root)
        val = 5;
    mp.Broadcast(val,root);
    REQUIRE(5 == val);
}

TEST_CASE("Broadcast a double"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0;
    double val=0,check_val=5.0 + 1.7e-13;
    if(mp.Rank() == root)
        val = check_val;
    mp.Broadcast(val,root);
    REQUIRE(check_val == Approx(val));
}

TEST_CASE("Broadcast vector of ints"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0;
    std::vector<int> vec(5,0);
    if(mp.Rank() == root)
        for(int i=0;i<5;i++)
            vec[i] = i;
    mp.Broadcast(vec,5,root);
    REQUIRE(5 == vec.size());
    for(int i=0;i<5;i++)
        REQUIRE(i == vec[i]);
}

TEST_CASE("Broadcast string"){
    std::string s = "dog";
    MessagePasser mp(MPI_COMM_WORLD);
    mp.Broadcast(s,0);
    REQUIRE(3 == s.size());
}

TEST_CASE("Broadcast a vector of strings"){
    std::vector<std::string> good_boys;
    MessagePasser mp(MPI_COMM_WORLD);
    if(mp.Rank() == 0){
        good_boys = std::vector<std::string>{"Spot", "Bojack", "Ace", "Bandit"};
    }

    mp.Broadcast(good_boys, 0);
    REQUIRE(good_boys[0] == "Spot");
    REQUIRE(good_boys[1] == "Bojack");
    REQUIRE(good_boys[2] == "Ace");
    REQUIRE(good_boys[3] == "Bandit");
}

TEST_CASE("Broadcast a vector of unspecified size"){
    MessagePasser mp(MPI_COMM_WORLD);

    int root = 0;
    std::vector<int> vec;
    if(mp.Rank() == root)
        for(int i=0;i<5;i++)
            vec.push_back(i);
    mp.Broadcast(vec,root);
    REQUIRE(5 == vec.size());
    for(int i=0;i<5;i++)
        REQUIRE(i == vec[i]);
}

TEST_CASE("Broadcast a set"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::set<int> my_set;
    if(mp.Rank() == 0)
        my_set.insert(mp.Rank());

    mp.Broadcast(my_set, 0);
    REQUIRE(my_set.size());
    REQUIRE(my_set.count(0) != 0);
}

TEST_CASE("Broadcast a map of trivially copyable"){
    MessagePasser mp(MPI_COMM_WORLD);
    std::map<int, std::array<double, 3>> my_map;
    if(mp.Rank() == 0)
        my_map[4] = std::array<double, 3>{9,7,6};

    mp.Broadcast(my_map, 0);
    REQUIRE(my_map.size() == 1);
    REQUIRE(my_map.at(4) == std::array<double, 3>{9,7,6});
}

TEST_CASE("Broadcast a bool"){
    MessagePasser mp(MPI_COMM_WORLD);
    bool good = false;
    if(mp.Rank() == 0)
        good = true;
    mp.Broadcast(good, 0);
    REQUIRE(good);
    if(mp.Rank() == 0)
        good = false;
    mp.Broadcast(good, 0);
    REQUIRE_FALSE(good);
}