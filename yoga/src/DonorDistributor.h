#pragma once
#include "DonorCollector.h"
#include "PortMapper.h"
#include "Receptor.h"
#include "ZMQMessager.h"

namespace YOGA {

class DonorDistributor {
  public:
    static void distribute(std::vector<Receptor>& receptorUpdates,
        std::shared_ptr<ZMQServerNameGenerator> server_names,PortMapper& portMapper) {
        auto ownerToReceptors = mapReceptorsToOwners(receptorUpdates);

        for (auto& stuffForOwner : ownerToReceptors) {
            int messageType = DonorCollector::Receptors;
            int owningRank = stuffForOwner.first;
            auto& receptors = stuffForOwner.second;
            ZMQMessager::Client client(server_names);
            int dciServerPort = portMapper.getPortNumber(owningRank, MessageTypes::DciUpdate);
            client.connectToServer(owningRank, dciServerPort);
            MessagePasser::Message msg;
            msg.pack(messageType);
            Receptor::pack(msg, receptors);
            client.send(msg);
            client.disconnectFromServer(owningRank, dciServerPort);
        }
    }

  //private:
    static std::map<int, std::vector<Receptor>> mapReceptorsToOwners(std::vector<Receptor>& receptors) {
        std::map<int, std::vector<Receptor>> m;
        for (auto& r : receptors) m[r.owner].push_back(r);
        return m;
    }
};
}
