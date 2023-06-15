#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <string>
#include <t-infinity/FilterFactory.h>
#include <t-infinity/VizFactory.h>
#include <t-infinity/VectorFieldAdapter.h>
#include <t-infinity/DistanceCalculator.h>
#include <t-infinity/QuiltTags.h>
#include <t-infinity/Snap.h>
#include <t-infinity/ScriptVisualization.h>
#include <t-infinity/FieldTools.h>
#include <parfait/FileTools.h>
#include <t-infinity/MeshExploder.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/MeshExtruder.h>
#include <memory>

using namespace inf;

class ExtrudeCommand : public SubCommand {
  public:
    std::string description() const override { return "Extrude 2D surfaces to 3D mesh."; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "default");
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter({"--tolerance"}, "The difference tolerance", false, "1e-10");
        m.addParameter({"o"}, "Output filename", false, "out.lb8.ugrid");
        m.addParameter({"--tags", "-t"}, "1,2,3 = plot only tags 1 2 and 3", false);
        m.addParameter({"--angle", "-a"}, "angle in degrees of rotation", true);
        m.addFlag({"--symmetric"},
                  "Extrude half the distance in each direction (instead of all the extrsion "
                  "distance in a single direction).");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = importMesh(m, mp);
        double angle = m.getDouble("angle");

        bool symmetric = m.has("symmetric");
        auto out = inf::extrude::extrudeAxisymmetric(
            mp, *mesh, inf::extrude::AXISYMMETRIC_X, angle, symmetric);

        inf::shortcut::visualize(m.get("o"), mp, out);
    }

  private:
    std::vector<int> getTags(const Parfait::CommandLineMenu& m) const {
        return Parfait::StringTools::parseIntegerList(m.get("tags"));
    }
};

CREATE_INF_SUBCOMMAND(ExtrudeCommand)