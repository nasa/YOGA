#include <parfait/MapbcWriter.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/AddMissingFaces.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/QuiltTags.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/ReorderMesh.h>
#include <t-infinity/FaceNeighbors.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/HangingEdgeRemesher.h>
#include <t-infinity/MapbcLumper.h>

using namespace inf;

class FixMeshCommand : public SubCommand {
  public:
    std::string description() const override { return "Try to fix problems with a mesh"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "fixed");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter(
            {"--missing-faces", "-f"}, "Create surface elements on missing faces.  Select int for new tag", false, "");
        m.addParameter({"--quilt-tags", "-q"}, "Merge multiple tags to one tag", false, "");
        m.addParameter({"--lump-bcs"}, "Lump bcs based on boundary name (from mapbc file)", false, "");
        m.addFlag({"--hang-edges", "--hanging-edges", "--hang-edge", "--hanging-edge"}, "Try to fix hanging edges");
        m.addFlag({"--reorder"}, "To reorder with Reverse Cuthill-McKee");
        m.addFlag({"--double-faces"}, "Check for multiply connected face neighbors");
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
        if (m.has("--quilt-tags") and not m.get("--quilt-tags").empty()) {
            auto mesh = importMesh(m, mp);
            auto tags = Parfait::StringTools::parseIntegerList(m.get("--quilt-tags"));
            mp_rootprint("merging tags %s\n", Parfait::to_string(tags).c_str());
            inf::quiltTagsAndCompact(mp, *mesh, {tags});
            inf::ParallelUgridExporter::write(output_meshname, *mesh, mp);
        } else if (m.has("--missing-faces") and not m.get("--missing-faces").empty()) {
            mp_rootprint("Adding missing faces\n");
            int new_tag = m.getInt("--missing-faces");
            auto mesh = importMesh(m, mp);
            auto fixed_mesh = inf::addMissingFaces(mp, *mesh, new_tag);
            inf::ParallelUgridExporter::write(output_meshname, *fixed_mesh, mp);
        } else if (m.has("lump-bcs")) {
            auto mapbc_filename = m.get("lump-bcs");
            PARFAIT_ASSERT((not mapbc_filename.empty()), "Empty mapbc filename given.");
            PARFAIT_ASSERT(Parfait::FileTools::doesFileExist(mapbc_filename), "mapbc file doesn't exist: "+mapbc_filename);
            Parfait::MapbcReader reader(m.get("lump-bcs"));
            auto bc = reader.getMap();
            auto new_tags = inf::groupTagsByBoundaryConditionName(bc);
            auto lumped_bc = inf::lumpBC(bc, new_tags);
            Parfait::writeMapbcFile("lumped.mapbc", lumped_bc);
            auto mesh = importMesh(m, mp);
            inf::remapSurfaceTags(*mesh, new_tags);
            inf::ParallelUgridExporter::write(output_meshname, *mesh, mp);
        } else if (m.has("--hang-edge")) {
            mp_rootprint("Trying to fix hanging edges\n");
            auto mesh = importMesh(m, mp);
            auto fixed_mesh = inf::HangingEdge::remesh(mp, mesh);
            inf::ParallelUgridExporter::write(output_meshname, *fixed_mesh, mp);
        } else if (m.has("--double-faces")) {
            auto mesh = importMesh(m, mp);
            auto face_neighbors = inf::FaceNeighbors::build(*mesh);
            for (int c = 0; c < mesh->cellCount(); c++) {
                std::set<int> found_neighbors;
                for (auto neighbor : face_neighbors[c]) {
                    if(neighbor < 0) continue;
                    if (found_neighbors.count(neighbor)) {
                        printf("Cell %d type %s is connected to neighbor %d type %s multiple times\n",
                               c,
                               inf::MeshInterface::cellTypeString(mesh->cellType(c)).c_str(),
                               neighbor,
                               inf::MeshInterface::cellTypeString(mesh->cellType(neighbor)).c_str());
                    }
                    found_neighbors.insert(neighbor);
                }
            }

        } else if (m.has("--reorder")) {
            mp_rootprint("Reordering the mesh using cell based Reverse Cuthill-McKee\n");
            auto mesh = importMesh(m, mp);
            auto reordered = inf::MeshReorder::reorderCells(mesh);
            reordered = inf::MeshReorder::reorderNodesBasedOnCells(reordered);

            long gid = 0;
            for (auto& pair : mesh->mesh.global_cell_id) {
                for (unsigned long i = 0; i < pair.second.size(); i++) {
                    mesh->mesh.global_cell_id.at(pair.first)[i] = gid++;
                }
            }
            gid = 0;
            for (long n = 0; n < mesh->nodeCount(); n++) {
                mesh->mesh.global_node_id[n] = gid++;
            }

            inf::ParallelUgridExporter::write(output_meshname, *reordered, mp);
        } else {
            PARFAIT_EXIT("No fix algorithm requested. Try --missing-faces ? ");
        }
    }
};

CREATE_INF_SUBCOMMAND(FixMeshCommand)
