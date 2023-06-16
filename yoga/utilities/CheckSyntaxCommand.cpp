#include <stdio.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <YogaConfiguration.h>
#include <parfait/FileTools.h>

using namespace YOGA;

class CheckSyntaxCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Check syntax for input files"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(inf::Alias::help(), inf::Help::help());
        m.addParameter({"config"},"check config", false);
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        if(m.has("config")){
            checkConfigFile(m.get("config"));
        }
        else{
            printf("%s\n", m.to_string().c_str());
        }
    }

  private:

    void checkConfigFile(std::string filename){
        auto s = Parfait::FileTools::loadFileToString(filename);
        YogaConfiguration config(s);
        printf("Config file ok.\n");
    }
};

CREATE_INF_SUBCOMMAND(CheckSyntaxCommand)
