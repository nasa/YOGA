#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/CompositeMeshBuilder.h>

using namespace inf;

class MergeCommand : public SubCommand {
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "ParfaitViz");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "out.lb8.ugrid");
        return m;
    }
     std::string description() const override {
         return "Merge multiple meshes into a single mesh";
     }

     void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh_names = m.getAsVector(Alias::mesh());
        if(mesh_names.size() == 1) {
            mp_rootprint("Error: Only one mesh was specified: %s\n", mesh_names.front().c_str());
            mp.Barrier();
            exit(1);
        }
        std::vector<std::shared_ptr<MeshInterface>> meshes;
        for(auto& name : mesh_names)
            meshes.push_back(importMesh(mp, name));

        auto merged = CompositeMeshBuilder::createCompositeMesh(mp, meshes);
         inf::shortcut::visualize(m.get(Alias::outputFileBase()), mp, merged, {}, m.get(Alias::postprocessor()));
     }

};

CREATE_INF_SUBCOMMAND(MergeCommand)
