#pragma once
#include <MessagePasser/MessagePasser.h>

class PortMapper {
  public:
    PortMapper() = delete;
    PortMapper(MessagePasser mp, const std::vector<int> myPortNumbers) {
        mp.Gather(myPortNumbers, portNumbersForRanks);
    }
    int getPortNumber(int rank, int channel) { return portNumbersForRanks[rank][channel]; }

  private:
    std::vector<std::vector<int>> portNumbersForRanks;
};
