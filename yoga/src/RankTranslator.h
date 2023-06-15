#pragma once

template <typename T>
class RankTranslator {
  public:
    RankTranslator(const std::vector<T>& ranks_per_comm)
        : ranks_per_comm(ranks_per_comm), rank_offsets(calcRankOffsets(ranks_per_comm)) {}

    T globalRank(T local_rank, int communicator_id) {
        throwIfBadInput(local_rank, communicator_id);
        return local_rank + rank_offsets[communicator_id];
    }

  private:
    const std::vector<T>& ranks_per_comm;
    std::vector<T> rank_offsets;

    std::vector<T> calcRankOffsets(const std::vector<T>& ranks_per_comm) {
        std::vector<T> offsets = {0};
        for (int i : ranks_per_comm) offsets.push_back(i + offsets.back());
        return offsets;
    }

    void throwIfBadInput(T local_rank, int communicator_id) {
        throwIfInvalidCommunicator(local_rank, communicator_id);
        throwIfInvalidRank(local_rank, communicator_id);
    }

    void throwIfInvalidCommunicator(T local_rank, int communicator_id) {
        try {
            ranks_per_comm.at(communicator_id);
        } catch (...) {
            throw std::domain_error("Invalid communicator id");
        }
    }

    void throwIfInvalidRank(T local_rank, int communicator_id) {
        if (local_rank >= ranks_per_comm[communicator_id]) throw std::logic_error("Invalid rank");
    }
};
