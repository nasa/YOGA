#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/AddMissingFaces.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/QuiltTags.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/ReorderMesh.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/HangingEdgeRemesher.h>
#include "t-infinity/MapbcLumper.h"

using namespace inf;

class ReorderCommand : public SubCommand {
  public:
    std::string description() const override { return "To Reorder a mesh with different algorithms"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "reordered");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter({"--algorithm", "-a"}, "[random, rcm, q]", true);
        m.addParameter({"--at"}, "[nodes, cells]", false, "nodes");
        m.addParameter({"-p"}, "The p value if using q reordering", false, "1");

        return m;
    }
    std::shared_ptr<TinfMesh> importMesh(const Parfait::CommandLineMenu& m, const MessagePasser& mp) const {
        auto dir = m.get(Alias::plugindir());
        auto mesh_loader_plugin_name = m.get(Alias::preprocessor());
        if (mp.NumberOfProcesses() == 1) {
            mp_rootprint("Detected serial.  Using S_PreProcessor");
            mesh_loader_plugin_name = "S_PreProcessor";
        }
        auto mesh_filename = m.get(Alias::mesh());
        mp_rootprint("Importing mesh: %s\n", mesh_filename.c_str());
        mp_rootprint("Using plugin: %s\n", mesh_loader_plugin_name.c_str());
        auto pp = getMeshLoader(dir, mesh_loader_plugin_name);
        auto imported_mesh = pp->load(mp.getCommunicator(), m.get(Alias::mesh()));
        return std::make_shared<TinfMesh>(imported_mesh);
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        std::string output_meshname = m.get(Alias::outputFileBase());
        auto algorithm_string = Parfait::StringTools::tolower(m.get("algorithm"));
        mp_rootprint("Reordering the mesh using cell based on %s\n", algorithm_string.c_str());
        auto mesh = importMesh(m, mp);
        MeshReorder::Algorithm algorithm = MeshReorder::Algorithm::RANDOM;
        if(algorithm_string == "rcm")
            algorithm = MeshReorder::RCM;
        else if(algorithm_string == "random")
            algorithm = MeshReorder::RANDOM;
        else if(algorithm_string == "q")
            algorithm = MeshReorder::Q;

        double p = m.getDouble("p");

        auto at = Parfait::StringTools::tolower(m.get("at"));
        std::shared_ptr<inf::TinfMesh> reordered;
        if(at == "nodes" or at == "node"){
            auto n2n = NodeToNode::build(*mesh);
            MeshReorder::writeAdjacency( "out.n2n.before", n2n);
            reordered = MeshReorder::reorderNodes(mesh, algorithm, p);
            n2n = NodeToNode::build(*reordered);
            MeshReorder::writeAdjacency(+ "out.n2n.after", n2n);

        } else if(at == "cells" or at == "cell"){
            auto c2c = CellToCell::build(*mesh);
            MeshReorder::writeAdjacency( "out.c2c.before", c2c);
            reordered = MeshReorder::reorderCells(mesh, algorithm, p);
            reordered = MeshReorder::reorderNodesBasedOnCells(reordered);
            c2c = CellToCell::build(*reordered);
            MeshReorder::writeAdjacency(+ "out.c2c.after", c2c);
        }

        long gid = 0;
        for (auto& pair : reordered->mesh.global_cell_id) {
            for (unsigned long i = 0; i < pair.second.size(); i++) {
                reordered->mesh.global_cell_id.at(pair.first)[i] = gid++;
            }
        }
        gid = 0;
        for (long n = 0; n < reordered->nodeCount(); n++) {
            reordered->mesh.global_node_id[n] = gid++;
        }

        ParallelUgridExporter::write(output_meshname, *reordered, mp);
    }
};

CREATE_INF_SUBCOMMAND(ReorderCommand)
