
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#include <array>
#include "../MessagePasser.h"
#include <RingAssertions.h>

TEST_CASE("vector of ints") {
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<int> vec;
    if (mp.Rank() == 0) {
        for (int i = 0; i < 9; i++) vec.push_back(i);
        for (int i = 1; i < mp.NumberOfProcesses(); i++) mp.Send(vec, 9, i);
    } else {
        mp.Recv(vec, 9, 0);
    }
    // check that everyone has the right size vector
    REQUIRE(9 == (int)vec.size());
    for (int i = 0; i < (int)vec.size(); i++) {
        REQUIRE(i == vec[i]);
    }
}

TEST_CASE("vector of floats") {
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<float> vec;
    float junk = 1.7e-5f;
    if (mp.Rank() == 0) {
        for (int i = 0; i < 9; i++) vec.push_back(junk + (float)i);
        for (int i = 1; i < mp.NumberOfProcesses(); i++) mp.Send(vec, 9, i);
    } else {
        mp.Recv(vec, 9, 0);
    }
    // check that everyone has the right size vector
    REQUIRE(9 == (int)vec.size());
    for (int i = 0; i < (int)vec.size(); i++) REQUIRE((junk + (float)i) == Approx(vec[i]));
}

TEST_CASE("Vector of doubles") {
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<double> vec;
    double junk = 1.7e-13;
    if (mp.Rank() == 0) {
        for (int i = 0; i < 9; i++) vec.push_back(junk + (double)i);
        for (int i = 1; i < mp.NumberOfProcesses(); i++) mp.Send(vec, 9, i);
    } else {
        mp.Recv(vec, 9, 0);
    }
    // check that everyone has the right size vector
    REQUIRE(9 == (int)vec.size());
    for (int i = 0; i < (int)vec.size(); i++) REQUIRE((junk + (double)i) == Approx(vec[i]));
}

TEST_CASE("vector of integers whose size is unknown to receiver") {
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<int> vec;
    if (mp.Rank() == 0) {
        for (int i = 0; i < 9; i++) vec.push_back(i);
        for (int i = 1; i < mp.NumberOfProcesses(); i++) mp.Send(vec, i);
    } else {
        mp.Recv(vec, 0);
    }
    // check that everyone has the right size vector
    REQUIRE(9 == (int)vec.size());
    for (int i = 0; i < (int)vec.size(); i++) REQUIRE(i == vec[i]);
}

TEST_CASE("Empty vector") {
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<int> vec;
    if (mp.Rank() == 0)
        for (int i = 1; i < mp.NumberOfProcesses(); i++) mp.Send(vec, i);
    else
        mp.Recv(vec, 0);
    // check that everyone has the right size vector
    REQUIRE(0 == (int)vec.size());
}

TEST_CASE("vector of std::arrays with send/recv") {
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<std::array<double, 2>> vec(9);
    if (mp.Rank() == 0) {
        for (int i = 0; i < 9; i++) {
            std::array<double, 2> v = {double(i), double(i)};
            vec[i] = v;
        }
        for (int i = 1; i < mp.NumberOfProcesses(); i++) mp.Send(vec, MessagePasser::bigToInt(vec.size()), i);
    } else {
        mp.Recv(vec, MessagePasser::bigToInt(vec.size()), 0);
    }
    // check that everyone has the right size vector
    REQUIRE(9 == (int)vec.size());
    for (int i = 0; i < (int)vec.size(); i++) {
        REQUIRE(double(i) == vec[i][0]);
        REQUIRE(double(i) == vec[i][1]);
    }
}

TEST_CASE("vector of std::arrays with send/recv - non-blocking") {
    MessagePasser mp(MPI_COMM_WORLD);

    std::vector<std::array<double, 2>> vec(9);

    auto status = mp.NonBlockingRecv(vec, 0);

    if (mp.Rank() == 0) {
        for (int i = 0; i < 9; i++) {
            std::array<double, 2> v = {double(i), double(i)};
            vec[i] = v;
        }
        auto v2 = vec;
        for (int i = 0; i < mp.NumberOfProcesses(); i++) {
            mp.Send(v2, i);
        }
    }

    status.wait();

    // check that everyone has the right size vector
    REQUIRE(9 == (int)vec.size());
    for (int i = 0; i < (int)vec.size(); i++) {
        REQUIRE(double(i) == vec[i][0]);
        REQUIRE(double(i) == vec[i][1]);
    }
}
