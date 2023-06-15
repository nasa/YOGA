#pragma once
#include <parfait/CommandLineMenu.h>
#include <MessagePasser/MessagePasser.h>
#include <string>
#include "CommonAliases.h"

namespace inf {
class SubCommand {
  public:
    virtual std::string description() const = 0;
    virtual void run(Parfait::CommandLineMenu m, MessagePasser mp) = 0;
    virtual Parfait::CommandLineMenu menu() const = 0;
    virtual ~SubCommand() = default;
};

class SubCommandFactory {
  public:
    virtual int subCommandCount() = 0;
    virtual std::shared_ptr<SubCommand> getSubCommand(int i) = 0;
    virtual ~SubCommandFactory() = default;
};
}

extern "C" {
std::shared_ptr<inf::SubCommandFactory> getSubCommandFactory();
}

#define CREATE_INF_SUBCOMMAND(name)                                                       \
    extern "C" {                                                                          \
    std::shared_ptr<inf::SubCommand> getSubCommand() { return std::make_shared<name>(); } \
    }
