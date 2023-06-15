#include <t-infinity/SubCommand.h>
#include <parfait/Namelist.h>
#include <parfait/FileTools.h>

using namespace inf;
using namespace Parfait;

class PatchCommand : public SubCommand {
  public:
    std::string description() const override { return "Apply namelist files as patches to a baseline nml"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"base","b"},"baseline nml file",true);
        m.addParameter({"patches","p"},"one or more nml files to apply",true);
        m.addParameter({"o"},"output filename",false,"out.nml");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto base = m.get("base");
        printf("Importing baseline namelist: %s\n",base.c_str());
        auto contents = FileTools::loadFileToString(base);
        printf("Contents:\n%s",contents.c_str());
        Namelist nml(FileTools::loadFileToString(base));
        for(auto patch :m.getAsVector("patches")){
            printf("Applying patch: %s\n",patch.c_str());
            contents = FileTools::loadFileToString(patch);
            printf("Contents:\n%s",contents.c_str());
            nml.patch(contents);
        }
        auto outfile = m.get("o");
        printf("Exporting: %s\n",outfile.c_str());
        FileTools::writeStringToFile(m.get("o"),nml.to_string());
    }

  private:

};

CREATE_INF_SUBCOMMAND(PatchCommand)
