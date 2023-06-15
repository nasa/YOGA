#pragma once
#include <MessagePasser/MessagePasser.h>
#include <mpi.h>
#include <array>
#include <map>
#include <vector>
#include "FaceNeighbors.h"
#include <t-infinity/MeshInterface.h>

namespace inf {
std::map<std::array<long, 2>, long> buildGlobalPairIds(
    MPI_Comm comm, const std::vector<std::array<long, 2>>& pairs, long max_number_of_pairs);

namespace impl_for_testing {
    std::map<int, std::vector<std::array<long, 2>>> queuePairsToSendToOwners(
        const std::vector<std::array<long, 2>>& pairs, std::function<int(long)> getOwner);
    std::set<std::array<long, 2>> flattenAndMakeUnique(
        const std::map<int, std::vector<std::array<long, 2>>>& incoming_pairs);
    long calcMyStartOffset(const MessagePasser& mp, long num_owned);
    std::map<std::array<long, 2>, long> assignIdsForOwnedPairs(
        const std::set<std::array<long, 2>>& pairs, long next_global_id);
}
}
