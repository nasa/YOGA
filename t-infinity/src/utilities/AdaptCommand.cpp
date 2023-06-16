#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/Snap.h>
#include <parfait/Dictionary.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/MeshAdaptionInterface.h>
#include <t-infinity/GeometryAssociationLoaderInterface.h>
#include <t-infinity/ByteStreamLoaderInterface.h>

using namespace inf;

class AdaptCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Adapt mesh with a metric"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(), Help::help());
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter({"metric"}, "input metric field (snap file)", true);
        m.addParameter({"sweeps", "s"}, "number of refine adaptation sweeps", false, "15");
        m.addHiddenFlag({"disable-curvature-constraint", "c"},
                        "disable the curvature constraint in refine.");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "adapted-mesh");
        return m;
    }

    MeshAdaptationInterface::MeshAssociatedWithGeometry loadMeshAssociatedWithGeometry(
        MPI_Comm comm, const std::string& mesh_filename, const std::string& plugin_dir) const {
        auto mesh = inf::getMeshLoader(plugin_dir, "RefinePlugins")->load(comm, mesh_filename);
        auto extension = Parfait::StringTools::getExtension(mesh_filename);
        std::shared_ptr<GeometryAssociationInterface> geometry = nullptr;
        if (extension == "meshb") {
            geometry = getGeometryAssociationLoader(plugin_dir, "RefinePlugins")
                           ->load(comm, mesh_filename, mesh);
        }
        return {mesh, geometry};
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto input_mesh_name = m.get(Alias::mesh());
        auto mesh_loader_plugin = selectDefaultPreProcessorFromMeshFilename(input_mesh_name);
        auto snap_file_name = m.get("metric");
        inf::Snap snap(mp.getCommunicator());
        auto output_mesh = m.get(Alias::outputFileBase());
        auto plugin_dir = m.get(Alias::plugindir());

        auto mesh_associated_with_geometry =
            loadMeshAssociatedWithGeometry(mp.getCommunicator(), input_mesh_name, plugin_dir);
        snap.addMeshTopologies(*mesh_associated_with_geometry.mesh);
        snap.load(snap_file_name);
        auto metric = snap.retrieve("metric");

        Parfait::Dictionary adaptation_options;
        adaptation_options["output_project"] = m.get("o");
        adaptation_options["sweeps"] = m.getInt("sweeps");
        if (m.has("disable-curvature-constraint"))
            adaptation_options["curvature_constraint"] = false;

        auto mesh_adaptation_plugin = getMeshAdaptationPlugin(plugin_dir, "RefinePlugins");
        std::shared_ptr<ByteStreamInterface> geometry_model = nullptr;

        if (mesh_associated_with_geometry.mesh_geometry_association != nullptr) {
            geometry_model = getByteStreamLoader(plugin_dir, "RefinePlugins")
                                 ->load(input_mesh_name, mp.getCommunicator());
        }

        if (mesh_associated_with_geometry.mesh_geometry_association != nullptr) {
            mesh_associated_with_geometry =
                mesh_adaptation_plugin->adapt(mesh_associated_with_geometry,
                                              geometry_model,
                                              mp.getCommunicator(),
                                              metric,
                                              adaptation_options.dump());
        } else {
            mesh_associated_with_geometry.mesh =
                mesh_adaptation_plugin->adapt(mesh_associated_with_geometry.mesh,
                                              mp.getCommunicator(),
                                              metric,
                                              adaptation_options.dump());
        }
    }

  private:
    std::shared_ptr<FieldInterface> loadMetricFromSnapFile(MessagePasser mp,
                                                           const MeshInterface& mesh,
                                                           const std::string snapfile) {
        Snap snap(mp.getCommunicator());
        snap.addMeshTopologies(mesh);
        snap.load(snapfile);
        auto available_fields = snap.availableFields();
        if (std::find(available_fields.begin(), available_fields.end(), "metric") ==
            available_fields.end()) {
            printf("Snap file does not contain a metric.\n");
            printf("Available fields:\n");
            for (auto s : available_fields) printf("   - %s\n", s.c_str());
            return nullptr;
        }
        return snap.retrieve("metric");
    }
};

CREATE_INF_SUBCOMMAND(AdaptCommand)
