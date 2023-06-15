#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/ElevateMesh.h>
#include "parfait/Checkpoint.h"

using namespace inf;

class ElevateMeshCommand : public SubCommand {
  public:
    std::string description() const override { return "Elevate a P1 mesh to a P2 mesh."; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "fixed");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        std::string output_filename = m.get(Alias::outputFileBase());
        mp_rootprint("Loading P1 mesh: %s\n", m.get(Alias::mesh()).c_str());
        auto p1_mesh = std::make_shared<TinfMesh>(shortcut::loadMesh(mp, m.get(Alias::mesh())));
        mp_rootprint("Done loading P1 mesh: %s\n", m.get(Alias::mesh()).c_str());

        mp_rootprint("Elevating to P2\n");
        auto p2_mesh = inf::elevateToP2(mp, *p1_mesh);
        mp_rootprint("Done elevating to P2\n");

        mp_rootprint("Exporting P2 mesh\n");
        inf::shortcut::visualize(output_filename, mp, p2_mesh);
        mp_rootprint("Done exporting P2 mesh\n");
    }
};

CREATE_INF_SUBCOMMAND(ElevateMeshCommand)
