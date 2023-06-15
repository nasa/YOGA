#pragma once
#include "t-infinity/SubCommand.h"

namespace inf {
class NullCommand : public SubCommand {
  public:
    std::string description() const override { return "null"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"null"}, "", true);
        return m;
    }
    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {}
};
}
