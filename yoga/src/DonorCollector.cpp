#include "DonorCollector.h"

namespace YOGA {

MessagePasser::Message DonorCollector::doWork(MessagePasser::Message& request) {
    int messageType = 0;
    request.unpack(messageType);
    if (Receptors == messageType) {
        std::vector<Receptor> newReceptors;
        Receptor::unpack(request, newReceptors);
        storeDonorUpdate(newReceptors);
    } else {
        throw std::logic_error("DonorCollector: Invalid message type");
    }
    MessagePasser::Message emptyReply;
    return emptyReply;
}

void DonorCollector::storeDonorUpdate(const std::vector<Receptor>& receptorUpdate) {
    receptors.insert(receptors.end(), receptorUpdate.begin(), receptorUpdate.end());
}

}
