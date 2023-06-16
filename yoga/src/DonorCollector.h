#pragma once
#include "MessageTypes.h"
#include "Receptor.h"

namespace YOGA {

class DonorCollector {
  public:
    enum MessageType { Receptors };
    std::vector<Receptor> receptors;

    MessagePasser::Message doWork(MessagePasser::Message& request);

  private:
    void storeDonorUpdate(const std::vector<Receptor>& receptorUpdate);
};
}
