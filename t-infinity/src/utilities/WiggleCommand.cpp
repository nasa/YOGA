#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <string>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/MeshConnectivity.h>
#include <t-infinity/TinfMesh.h>
#include <parfait/Wiggle.h>
#include <t-infinity/ParallelUgridExporter.h>
#include "t-infinity/Wiggle.h"

using namespace inf;

class WiggleCommand : public SubCommand {
  public:
    std::string description() const override {
        return "Wiggle points in mesh.  Can increase dissipation/robustness for coarse Cartesian "
               "aligned meshes.";
    }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(
            {"percent"}, "wiggle points by percent of shortest connected edge", false, "1.0");
        m.addParameter({"o"}, "output grid name", false, "out.lb8.ugrid");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = std::make_shared<TinfMesh>(importMesh(m, mp));
        double percent = m.getDouble("percent");
        inf::wiggleNodes(mp, *mesh, percent);
        ParallelUgridExporter::write(m.get("o"), *mesh, MPI_COMM_WORLD);
    }

  private:
};

CREATE_INF_SUBCOMMAND(WiggleCommand)