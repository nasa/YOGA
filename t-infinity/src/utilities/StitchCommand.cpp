#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/AddMissingFaces.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/QuiltTags.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/ReorderMesh.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/HangingEdgeRemesher.h>
#include <t-infinity/StitchMesh.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include "t-infinity/MapbcLumper.h"

using namespace inf;

class StitchMeshCommand : public SubCommand {
  public:
    std::string description() const override {
        return "Try to stitch a list of meshes";
    }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "stitched.lb8.ugrid");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        return m; }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        std::string output_meshname = m.get(Alias::outputFileBase());
        auto mesh_names = m.getAsVector(Alias::mesh());
        std::vector<std::shared_ptr<inf::MeshInterface>> meshes;
        for(auto name : mesh_names) {
            auto mesh = importMesh(name, m, mp);
            meshes.push_back(mesh);
        }
        auto out_mesh = inf::StitchMesh::stitchMeshes(meshes);
        inf::shortcut::visualize(m.get("o"), mp, out_mesh);
    }
};

CREATE_INF_SUBCOMMAND(StitchMeshCommand)
