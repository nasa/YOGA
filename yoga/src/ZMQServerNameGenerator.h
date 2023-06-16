#pragma once
#include <MessagePasser/MessagePasser.h>
#include <string>
#include <vector>
#include <unistd.h>

namespace YOGA {

class ZMQServerNameGenerator {
  public:
    ZMQServerNameGenerator(MessagePasser mp)
        : serverNames(buildListOfServerNames(mp, machineName())) {
    }

    std::string getServerNameForRank(int i) { return serverNames[i]; }

  private:
    const std::vector<std::string> serverNames;

    std::vector<std::string> buildListOfServerNames(MessagePasser mp, std::string myName) {
        int nproc = mp.NumberOfProcesses();
        int rank = mp.Rank();
        std::vector<std::string> machineNames(nproc, "");
        machineNames[rank] = myName;
        for (int i = 0; i < nproc; ++i) {
            std::vector<char> tmp;
            if (rank == i) tmp.insert(tmp.begin(), myName.begin(), myName.end());
            mp.Broadcast(tmp, i);
            machineNames[i] = std::string(tmp.begin(), tmp.end());
        }
        return machineNames;
    }

    std::string machineName(){
        char s[256];
        gethostname(s, 256);
        return s;
    }

};

}
