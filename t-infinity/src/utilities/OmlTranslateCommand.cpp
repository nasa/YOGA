#include <parfait/Dictionary.h>
#include <parfait/OmlParser.h>
#include <parfait/FileTools.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>

using namespace inf;

class OmlTranslateCommand : public SubCommand {
  public:
    std::string description() const override { return "Translate poml files to json"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "out.json");
        m.addParameter({"--input", "-i"}, "Input poml formatted file", true);
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto input = m.get("i");
        auto contents = Parfait::FileTools::loadFileToString(input);
        auto dictionary = Parfait::OmlParser::parse(contents);
        auto output_contents = dictionary.dump(4);
        Parfait::FileTools::writeStringToFile(m.get("o"), output_contents);
    }
};

CREATE_INF_SUBCOMMAND(OmlTranslateCommand)
