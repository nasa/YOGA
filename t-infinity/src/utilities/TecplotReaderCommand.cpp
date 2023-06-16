#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/PluginLoader.h>
#include <t-infinity/Shortcuts.h>
#include <parfait/TecplotBinaryReader.h>

using namespace inf;
class TecplotReaderCommand : public SubCommand {
  public:
    std::string description() const override {
        return "To debug tecplot binary files.";
    }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"--input", "-i"}, "Input tecplot file", true);
        return m;
    }
    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        Parfait::tecplot::BinaryReader reader(m.get("i"));
        reader.read();
    }
};

CREATE_INF_SUBCOMMAND(TecplotReaderCommand)
