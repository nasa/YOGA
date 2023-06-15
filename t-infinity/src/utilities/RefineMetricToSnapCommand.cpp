#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/Shortcuts.h>

using namespace inf;

class RefineMetricToSnapCommand : public SubCommand {
  public:
    std::string description() const override { return "convert a 6-element vector (metric field) from refine solb to a snap file"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(),Help::help());
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::inputFile(),"solb file with metric from refine",true);
        m.addParameter(Alias::outputFileBase(),Help::outputFileBase(),false,"metric");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto solb_name = m.get(Alias::inputFile());
        auto mesh_name = m.get(Alias::mesh());
        auto out_name = m.get(Alias::outputFileBase());
        auto mesh = shortcut::loadMesh(mp,mesh_name);
        std::vector<std::shared_ptr<FieldInterface>> fields;
        inf::appendFieldsFromSolb(mp, solb_name,*mesh,m,fields);
        if(fields.size() != 1){
            printf("Expected 1 field with a metric in the solb file, but found %li\n",fields.size());
        } else {
            Snap snap(mp);
            snap.addMeshTopologies(*mesh);
            snap.add(fields.front());
            snap.writeFile(out_name);
        }
    }
};

CREATE_INF_SUBCOMMAND(RefineMetricToSnapCommand)
