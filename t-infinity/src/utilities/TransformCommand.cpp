#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/PluginLoader.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/MeshMover.h>
#include <t-infinity/SurfaceElementWinding.h>
#include <t-infinity/SurfaceNeighbors.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/CommonSubcommandFunctions.h>

using namespace Parfait::StringTools;

namespace inf  {
class TransformCommand : public SubCommand {
  public:
    std::string description() const override { return "transform a grid and export to file"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "default");
        m.addParameter({"-o"}, "output file based on extension", false, "out.vtk");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter({"translate"}, "Translate grid in <X,Y,Z>", false);
        m.addParameter({"scale"}, "Scale the grid about 0,0,0", false);
        m.addParameter({"scale-x"}, "Scale the grid x-axis about 0,0,0", false);
        m.addParameter({"scale-y"}, "Scale the grid y-axis about 0,0,0", false);
        m.addParameter({"scale-z"}, "Scale the grid z-axis about 0,0,0", false);
        m.addParameter({"rotate-x"}, "Rotate about x-axis by <degrees>", false);
        m.addParameter({"rotate-y"}, "Rotate about y-axis by <degrees>", false);
        m.addParameter({"rotate-z"}, "Rotate about z-axis by <degrees>", false);
        m.addParameter({"reflect-x"},"reflect about yz plane at given x value",false);
        m.addParameter({"reflect-y"},"reflect about xz plane at given y value",false);
        m.addParameter({"reflect-z"},"reflect about xy plane at given z value",false);
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = importMesh(m, mp);
        if (m.has("translate") or m.has("rotate-x") or m.has("rotate-y") or m.has("rotate-z"))
            mesh = moveMesh(mesh, m, mp);
        else if(m.has("reflect-x"))
            mesh = MeshMover::reflect(mesh,{1,0,0},m.getDouble("reflect-x"));
        else if(m.has("reflect-y"))
            mesh = MeshMover::reflect(mesh,{0,1,0},m.getDouble("reflect-y"));
        else if(m.has("reflect-z"))
            mesh = MeshMover::reflect(mesh,{0,0,1},m.getDouble("reflect-z"));

        Parfait::Point<double> scaling(1, 1, 1);
        if (m.has("scale")) {
            scaling[0] = m.getDouble("scale");
            scaling[1] = m.getDouble("scale");
            scaling[2] = m.getDouble("scale");
        }
        if (m.has("scale-x")) scaling[0] = m.getDouble("scale-x");
        if (m.has("scale-y")) scaling[1] = m.getDouble("scale-y");
        if (m.has("scale-z")) scaling[2] = m.getDouble("scale-z");
        mesh = scaleMesh(mesh, scaling);

        auto output_filename = m.get("-o");
        mp_rootprint("exporting to: %s\n", output_filename.c_str());
        auto postprocessor_name = m.get(Alias::postprocessor());
        postprocessor_name = selectDefaultPostProcessorFromMeshFilename(output_filename);
        inf::shortcut::visualize(output_filename, mp, mesh,{},postprocessor_name);
    }

    static std::shared_ptr<MeshInterface> scaleMesh(std::shared_ptr<MeshInterface> mesh_in,
                                                    Parfait::Point<double> scaling) {
        if (scaling.approxEqual(Parfait::Point<double>{1, 1, 1})) return mesh_in;
        auto mesh = std::make_shared<TinfMesh>(mesh_in);
        for (int n = 0; n < mesh->nodeCount(); n++) {
            mesh->mesh.points[n][0] *= scaling[0];
            mesh->mesh.points[n][1] *= scaling[1];
            mesh->mesh.points[n][2] *= scaling[2];
        }
        return mesh;
    }
    static std::shared_ptr<MeshInterface> moveMesh(std::shared_ptr<MeshInterface> mesh,
                                                   const Parfait::CommandLineMenu& m,
                                                   const MessagePasser& mp) {
        Parfait::MotionMatrix motion;
        if (m.has("translate")) {
            auto translation = m.getAsVector("translate");
            std::array<double, 3> t = {toDouble(translation[0]), toDouble(translation[1]), toDouble(translation[2])};
            mp_rootprint("Translating : %f [x] + %f [y] + %f [z]\n", t[0], t[1], t[2]);
            motion.addTranslation(t.data());
        }
        if (m.has("rotate-x")) {
            auto angle = m.getDouble("rotate-x");
            mp_rootprint("Rotating X : %f (deg)\n", angle);
            motion.addRotation({0, 0, 0}, {1, 0, 0}, angle);
        }
        if (m.has("rotate-y")) {
            auto angle = m.getDouble("rotate-y");
            mp_rootprint("Rotating Y : %f (deg)\n", angle);
            motion.addRotation({0, 0, 0}, {0, 1, 0}, angle);
        }
        if (m.has("rotate-z")) {
            auto angle = m.getDouble("rotate-z");
            mp_rootprint("Rotating Z : %f (deg)\n", angle);
            motion.addRotation({0, 0, 0}, {0, 0, 1}, angle);
        }
        return MeshMover::move(mesh, motion);
    }
};
}

CREATE_INF_SUBCOMMAND(inf::TransformCommand)
