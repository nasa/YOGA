#include <parfait/GraphColoring.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/GlobalToLocal.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/CommonSubcommandFunctions.h>

using namespace inf;
class MeshColorCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Color cells in a mesh"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(inf::Alias::plugindir(), inf::Help::plugindir(), false, inf::getPluginDir());
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "CC_ReProcessor");
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "ParfaitViz");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "out");
        m.addParameter({"a", "algorithm"}, "Coloring algorithm [balanced, greedy]", false, "balanced");
        m.addParameter({"--at"}, "[nodes, cells]", false, "cells");
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        return m;
    }
    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = importMesh(m, mp);

        std::vector<std::vector<int>> graph;
        std::vector<long> gids;
        std::vector<int> owners;

        std::string at = Parfait::StringTools::tolower(m.get("at"));
        PARFAIT_ASSERT(at == "nodes" or at == "cells", "Can only compute colors at nodes or cells");

        if(at == "cells") {
            graph = inf::FaceNeighbors::buildOwnedOnly(*mesh);
            gids = inf::LocalToGlobal::buildCell(*mesh);
            owners = inf::getCellOwners(*mesh);
        } else if(at == "nodes") {
            graph = inf::NodeToNode::build(*mesh);
            gids = inf::LocalToGlobal::buildNode(*mesh);
            owners = inf::getNodeOwners(*mesh);
        }

        Parfait::GraphColoring::Algorithm algorithm = Parfait::GraphColoring::BALANCED;
        if(m.has("algorithm")) {
            if(Parfait::StringTools::tolower(m.get("algorithm")) == "greedy"){
                mp_rootprint("Using greedy algorithm\n");
                algorithm = Parfait::GraphColoring::GREEDY;
            } else {
                mp_rootprint("Using balanced algorithm\n");
            }
        } else {
            mp_rootprint("Algorithm not selected, using balanced\n");
        }
        auto colors = Parfait::GraphColoring::color(mp, graph, gids, owners, algorithm);
        Parfait::GraphColoring::printStatistics(mp, graph, owners, colors);
        bool coloring_valid = Parfait::GraphColoring::checkColoring(mp, graph, owners, colors);
        mp_rootprint("Coloring is %svalid\n", (coloring_valid) ? ("") : ("NOT "));

        auto output_base = m.get(Alias::outputFileBase());
        auto colors_double = Parfait::VectorTools::to_double(colors);
        if(at == "cells") {
            inf::shortcut::visualize_volume_at_cells(output_base, mp, mesh,{colors_double}, {"load-balanced-colors"});
        } else if(at == "nodes") {
            inf::shortcut::visualize(output_base, mp, mesh,inf::FieldAttributes::Node(), {colors_double}, std::vector<std::string>{"load-balanced-colors"});
        }
    }
};

CREATE_INF_SUBCOMMAND(MeshColorCommand)
