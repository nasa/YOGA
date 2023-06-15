
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
#include "../MessagePasser.h"
#include <RingAssertions.h>

TEST_CASE("SumIntegers") {
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);
    int root = 0;
    int value = mp->Rank();
    int psum = 0;
    psum = mp->ParallelSum(value, root);
    if (mp->Rank() == root) {
        int sum = 0;
        for (int i = 0; i < mp->NumberOfProcesses(); i++) sum += i;
        REQUIRE(sum == psum);
    }
}

TEST_CASE("SumFloats") {
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);
    int root = 0;
    float junk = 1.7e-1f;
    float value = junk + (float)mp->Rank();
    float psum = 0.0;
    psum = mp->ParallelSum(value, root);
    if (mp->Rank() == root) {
        float sum = 0.0;
        for (int i = 0; i < mp->NumberOfProcesses(); i++) sum += junk + (float)i;
        REQUIRE(sum == Approx(psum));
    }
}

TEST_CASE("SumDoubles") {
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    int root = 0;
    double junk = 1.7e-9;
    double value = junk + (double)mp->Rank();
    double psum = 0.0;
    psum = mp->ParallelSum(value, root);
    if (mp->Rank() == root) {
        double sum = 0.0;
        for (int i = 0; i < mp->NumberOfProcesses(); i++) sum += junk + (double)i;
        REQUIRE(sum == Approx(psum));
    }
}

TEST_CASE("ElementalSumDoubles") {
    auto mp = std::make_shared<MessagePasser>(MPI_COMM_WORLD);

    int root = 0;
    std::vector<double> psum(mp->NumberOfProcesses(), 0.0);
    psum[mp->Rank()] = double(mp->Rank());
    psum[0] = 1.0;
    mp->ElementalSum(psum, root);
    if (mp->Rank() == root) {
        for (int i = 1; i < mp->NumberOfProcesses(); i++) {
            auto expected = double(i);
            REQUIRE(expected == Approx(psum[i]));
        }
        REQUIRE(double(mp->NumberOfProcesses()) == Approx(psum[0]));
    } else {
        REQUIRE(1.0 == Approx(psum[0]));
    }
}

TEST_CASE("ParallelAvg"){
    auto mp = MessagePasser(MPI_COMM_WORLD);
    double one = 1.0;
    auto avg = mp.ParallelAverage(one);
    REQUIRE(avg == 1.0);

    double rank = mp.Rank();
    double expected_avg = 0.0;
    for(int r = 0; r < mp.NumberOfProcesses(); r++){
        expected_avg += r;
    }
    expected_avg /= (double)mp.NumberOfProcesses();

    REQUIRE(expected_avg == mp.ParallelAverage(rank));
}
