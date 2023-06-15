#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <string>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/VizFactory.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/MeshLoader.h>
#include <t-infinity/Snap.h>
#include <t-infinity/ScriptVisualization.h>
#include <t-infinity/FieldTools.h>
#include <parfait/FileTools.h>
#include <t-infinity/MeshExploder.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/MeshSanityChecker.h>
#include <t-infinity/CommonSubcommandFunctions.h>

using namespace inf;

class ValidateCommand : public SubCommand {
  public:
    std::string description() const override { return "To run mesh validation checks on a given mesh."; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "CC_ReProcessor");
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "ParfaitViz");
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(
            {"--nishikawa-p", "-p"}, "The exponent used in the Nishikawa centroid calculation", false, "2.0");
        m.addHiddenFlag({"force"}, "force use of user specified pre-processor");
        m.addFlag({"--check-colocated-centroids", "-c"}, "Additionally check if centroids in a n2c stencil overlap");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = importMesh(m, mp);
        if (inf::MeshSanityChecker::isMeshValid(*mesh, mp)) {
            mp_rootprint("\n\nSuccess!  Mesh has passed all mesh checks.\n");
        } else {
            mp_rootprint("\n\n\n---------------------------\n");
            mp_rootprint("Failure!  Mesh has failed at least some of the checks. \n");
            mp_rootprint("If individual bad cells were found a .vtk files were written of that cell check with.\n");
            mp_rootprint("  ls *.vtk\n\n");
            mp_rootprint("If stencils of bad stencils were found you can visualize them with:\n");
            mp_rootprint("  inf plot -m %s --cell-list bad_stencils.txt\n\n", m.get(Alias::mesh().front()).c_str());
        }

        if (m.has("--check-colocated-centroids")) {
            double hiro_p = m.getDouble("-p");
            auto n2c = inf::NodeToCell::build(*mesh);
            auto nodal_avg_centroid_distance =
                inf::MeshSanityChecker::minimumCellCentroidDistanceInNodeStencil(*mesh, n2c);
            auto nodal_hiro_centroid_distance =
                inf::MeshSanityChecker::minimumCellHiroCentroidDistanceInNodeStencil(*mesh, n2c, hiro_p);
            auto min_avg_distance = inf::FieldTools::findNodalMin(mp, *mesh, nodal_avg_centroid_distance);
            auto min_hiro_distance = inf::FieldTools::findNodalMin(mp, *mesh, nodal_hiro_centroid_distance);

            mp_rootprint("Minimum centroid distance in a node to cell stencil is:\n")
                mp_rootprint("  %e (centroid as average of nodes)\n", min_avg_distance);
            mp_rootprint("  %e (centroid using Nishikawa method with p = %1.2lf\n", min_hiro_distance, hiro_p);

            inf::shortcut::visualize_at_nodes("node-neighbor-centroid-min-distance",
                                                   mp,
                                                   mesh,
                                                   {nodal_avg_centroid_distance, nodal_hiro_centroid_distance},
                                                   {"min-centroid-dist.avg", "min-centroid-dist.nishikawa"});
        }
    }
};

CREATE_INF_SUBCOMMAND(ValidateCommand)