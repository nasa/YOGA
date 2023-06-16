
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
#include <RingAssertions.h>
#include <vector>
#include <MessagePasser/MessagePasser.h>
#include <map>
#include <parfait/SyncPattern.h>

TEST_CASE("Sync Pattern Exists") {
    auto mp = MessagePasser(MPI_COMM_WORLD);

    std::vector<long> have, need;
    if (mp.Rank() == 0) {
        have = std::vector<long>{0, 1, 2, 3};
        need = std::vector<long>{4, 5, 6, 7};
    } else if (mp.Rank() == 1) {
        have = std::vector<long>{4, 5, 6, 7};
        need = std::vector<long>{0, 1, 2, 3};
    }

    Parfait::SyncPattern syncPattern = Parfait::SyncPattern(mp, have, need);
    if (mp.NumberOfProcesses() < 2) {
        REQUIRE_THROWS(syncPattern.send_to.at(1));
        REQUIRE_THROWS(syncPattern.receive_from.at(1));
    } else {
        if (mp.Rank() == 0) {
            REQUIRE(syncPattern.send_to.at(1).size() == 4);
            REQUIRE(syncPattern.receive_from.at(1).size() == 4);
        } else if (mp.Rank() == 1) {
            REQUIRE(syncPattern.send_to.at(0).size() == 4);
            REQUIRE(syncPattern.receive_from.at(0).size() == 4);
        }
    }
}

TEST_CASE("build send_to_length") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() < 2) return;

    std::vector<long> have, need;
    std::vector<int> need_from;
    if (mp.Rank() == 0) {
        have = std::vector<long>{0, 1, 2, 3};
        need = std::vector<long>{4, 5, 6, 7};
        need_from = std::vector<int>{1, 1, 1, 1};
    } else if (mp.Rank() == 1) {
        have = std::vector<long>{4, 5, 6, 7};
        need = std::vector<long>{0, 1, 2, 3};
        need_from = std::vector<int>{0, 0, 0, 0};
    }

    auto send_to_length = Parfait::SyncPattern::determineHowManyISendToEachRank(mp, need_from);
    REQUIRE((int)send_to_length.size() == mp.NumberOfProcesses());
    if (mp.Rank() == 0) {
        REQUIRE(send_to_length[0] == 0);
        REQUIRE(send_to_length[1] == 4);
    } else if (mp.Rank() == 1) {
        REQUIRE(send_to_length[0] == 4);
        REQUIRE(send_to_length[1] == 0);
    }
}

TEST_CASE("Sync Pattern faster build if you know who owns your needs") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() < 2) return;

    std::vector<long> have, need;
    std::vector<int> need_from;
    if (mp.Rank() == 0) {
        have = std::vector<long>{0, 1, 2, 3};
        need = std::vector<long>{4, 5, 6, 7};
        need_from = std::vector<int>{1, 1, 1, 1};
    } else if (mp.Rank() == 1) {
        have = std::vector<long>{4, 5, 6, 7};
        need = std::vector<long>{0, 1, 2, 3};
        need_from = std::vector<int>{0, 0, 0, 0};
    }

    Parfait::SyncPattern syncPattern = Parfait::SyncPattern(mp, have, need, need_from);
    if (mp.NumberOfProcesses() < 2) {
        REQUIRE_THROWS(syncPattern.send_to.at(1));
        REQUIRE_THROWS(syncPattern.receive_from.at(1));
    } else {
        if (mp.Rank() == 0) {
            REQUIRE(syncPattern.send_to.at(1).size() == 4);
            REQUIRE(syncPattern.receive_from.at(1).size() == 4);
        } else if (mp.Rank() == 1) {
            REQUIRE(syncPattern.send_to.at(0).size() == 4);
            REQUIRE(syncPattern.receive_from.at(0).size() == 4);
        }
    }
}

TEST_CASE("SyncPattern can send/recv when have/need overlap") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    std::vector<long> have = {0, 1, 2};
    std::vector<long> need = {0, 1, 2};
    SECTION("have/need") {
        Parfait::SyncPattern sync_pattern(mp, have, need);
        REQUIRE(std::vector<long>{0, 1, 2} == sync_pattern.send_to.at(mp.Rank()));
        REQUIRE(std::vector<long>{0, 1, 2} == sync_pattern.receive_from.at(mp.Rank()));
    }
    SECTION("have/need/need_from") {
        std::vector<int> need_from = {mp.Rank(), mp.Rank(), mp.Rank()};
        Parfait::SyncPattern sync_pattern(mp, have, need, need_from);
        REQUIRE(std::vector<long>{0, 1, 2} == sync_pattern.send_to.at(mp.Rank()));
        REQUIRE(std::vector<long>{0, 1, 2} == sync_pattern.receive_from.at(mp.Rank()));
    }
}

TEST_CASE("A snapshot of a failure seen in a mesh shuffle test. Verifying it isn't a problem with sync pattern") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 2) return;
    std::vector<long> have, need;
    std::vector<int> need_from;
    if (mp.Rank() == 0) {
        have = std::vector<long>{0, 1, 4, 5, 8, 9, 12, 13};
        need = std::vector<long>{2, 6, 10, 14};
        need_from = std::vector<int>{1, 1, 1, 1};
    } else {
        have = std::vector<long>{2, 3, 6, 7, 10, 11, 14, 15};
        need = std::vector<long>{1, 5, 9, 13};
        need_from = std::vector<int>{0, 0, 0, 0};
    }

    Parfait::SyncPattern sync_pattern(mp, have, need, need_from);
    REQUIRE(sync_pattern.receive_from.size() == 1);
    REQUIRE(sync_pattern.send_to.size() == 1);
    if (mp.Rank() == 0) {
        REQUIRE(sync_pattern.receive_from.at(1).size() == 4);
        REQUIRE(sync_pattern.receive_from.at(1)[0] == 2);
        REQUIRE(sync_pattern.receive_from.at(1)[1] == 6);
        REQUIRE(sync_pattern.receive_from.at(1)[2] == 10);
        REQUIRE(sync_pattern.receive_from.at(1)[3] == 14);
    } else {
        REQUIRE(sync_pattern.receive_from.at(0).size() == 4);
        REQUIRE(sync_pattern.receive_from.at(0)[0] == 1);
        REQUIRE(sync_pattern.receive_from.at(0)[1] == 5);
        REQUIRE(sync_pattern.receive_from.at(0)[2] == 9);
        REQUIRE(sync_pattern.receive_from.at(0)[3] == 13);
    }
}