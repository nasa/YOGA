#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/Cell.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/MeshMirror.h>

using namespace inf;

class MirrorCommand : public SubCommand {
  public:
    std::string description() const override { return "Mirror a mesh based on a collection of mesh tags that define a plane."; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter({"tags", "t"}, "Boundary tags of symmetry condition", true);
        m.addParameter(Alias::outputFileBase(), "Output mesh filename", false, "mirrored.lb8.ugrid");
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "default");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = importMesh(m, mp);

        auto v = inf::getTags(m);
        std::set<int> mirror_tags(v.begin(), v.end());
        mesh = MeshMirror::mirrorViaTags(mp, mesh, mirror_tags).getMesh();

        auto output_filename = m.get(Alias::outputFileBase());
        auto postprocessor_name = m.get(Alias::postprocessor());
        postprocessor_name = selectDefaultPostProcessorFromMeshFilename(output_filename);
        inf::shortcut::visualize(output_filename, mp, mesh, {}, postprocessor_name);
    }

  private:
    std::shared_ptr<MeshInterface> importMesh(const Parfait::CommandLineMenu& m, const MessagePasser& mp) {
        std::string dir = inf::getPluginDir();
        if (m.has(Alias::plugindir())) {
            dir = m.get(Alias::plugindir());
        }
        auto mesh_loader_plugin_name = m.get(Alias::preprocessor());
        auto mesh_filename = m.get(Alias::mesh());
        if (mesh_loader_plugin_name == "default") {
            mesh_loader_plugin_name = selectDefaultPreProcessorFromMeshFilename(mesh_filename);
        }
        bool verbose = false;
        if (m.has("verbose")) verbose = true;
        return inf::importMesh(mp, mesh_filename, mesh_loader_plugin_name, verbose, dir);
    }
};

CREATE_INF_SUBCOMMAND(MirrorCommand)
