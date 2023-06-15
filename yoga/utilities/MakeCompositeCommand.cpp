#include "t-infinity/SubCommand.h"
#include <t-infinity/CommonAliases.h>
#include <t-infinity/MeshMover.h>
#include <t-infinity/MotionMatrixParser.h>
#include <parfait/FileTools.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/MeshLoader.h>
#include <t-infinity/MeshHelpers.h>
#include <t-infinity/MapBcFileCombiner.h>
#include <t-infinity/CompositeMeshBuilder.h>
#include <t-infinity/MapbcLumper.h>
#include "MovingBodyParser.h"
#include "RootPrinter.h"
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/QuiltTags.h>
#include "PartVectorIO.h"
#include "Diagnostics.h"
#include "PartitionViz.h"

using namespace YOGA;

namespace inf  {
class MakeCompositeCommand : public SubCommand {
  public:
    std::string description() const override { return "combine multiple grids into a single one"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(), Help::help());
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter({"o"}, "output grid name (with extension)", false, "composite.b8.ugrid");
        m.addParameter(Alias::inputFile(), "name of composite builder script", true);
        m.addParameter({"imesh"}, "Name of imesh file for FUN3D", false, "imesh.dat");
        m.addParameter({"part"},"export fun3d-partition-vector file",false);
        m.addFlag({"swap"},"swap bytes when exporting partition file");
        m.addFlag({"mapbc-only"},"create composite.mapbc file without processing the grid");
        m.addFlag({"lump-bcs"},"combine boundary tags with matching names into single tag");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        if (m.has(Alias::help())) {
            printf("%s\n", m.to_string().c_str());
            return;
        }
        RootPrinter rp(mp.Rank());
        bool should_lump = m.has("lump-bcs");
        auto config_filename = m.get(Alias::inputFile());
        rp.print("Loading: "+config_filename+"\n");
        auto config = Parfait::FileTools::loadFileToString(config_filename);
        rp.print("Extracting grid names and transformations\n");
        auto grid_entries = inf::MotionParser::parse(config);
        std::string composite_grid_name = m.get(Alias::outputFileBase());
        std::vector<std::string> ugrid_extensions {"b8","lb8","ugrid"};
        std::string composite_grid_base = Parfait::StringTools::stripExtension(composite_grid_name,ugrid_extensions);

        if(mp.Rank() == 0){
            bool should_lump = m.has("lump-bcs");
            readAndCombineMapbcFiles(grid_entries,composite_grid_base, should_lump);
        }

        if(m.has("mapbc-only"))
            return;

        auto mesh_loader_plugin_name = m.get(Alias::preprocessor());
        auto pp = inf::getMeshLoader(inf::getPluginDir(), mesh_loader_plugin_name);
        std::vector<std::shared_ptr<inf::MeshInterface>> meshes;




        for (auto entry : grid_entries) {
            auto mesh = pp->load(MPI_COMM_WORLD, entry.grid_name);
            mesh = inf::MeshMover::move(mesh, entry.motion_matrix);
            if(should_lump){
                auto bc = Parfait::MapbcParser::parse(Parfait::FileTools::loadFileToString(entry.mapbc_name));
                auto old_to_new = groupTagsByBoundaryConditionName(bc);
                auto tinf_mesh = std::make_shared<TinfMesh>(mesh);
                remapSurfaceTags(*tinf_mesh, old_to_new);
                mesh = tinf_mesh;
            }
            meshes.push_back(mesh);
        }

        auto imesh_filename = m.get("--imesh");
        writeFUN3DImeshFile(imesh_filename, meshes, mp);



        auto mesh = inf::CompositeMeshBuilder::createCompositeMesh(MPI_COMM_WORLD, meshes);
        if(m.has("part")){
            auto filename = m.get("part");
            bool swap_bytes = m.has("swap");
            writeFUN3DPartitionFile(mp,filename,*mesh,swap_bytes);
            visualizePartitionExtents(mp,"rcb_partition_extents.vtk",*mesh);
            visualizePartitions(mp,"volume_partitions.vtk",mesh);
        }
        else {
            inf::ParallelUgridExporter::write(composite_grid_name, *mesh, MPI_COMM_WORLD, false);
        }
    }

    void writeFUN3DImeshFile(const std::string& filename,
                             const std::vector<std::shared_ptr<MeshInterface>>& meshes,
                             MessagePasser mp) const {
        std::vector<long> node_counts;
        for (auto& mesh : meshes) node_counts.push_back(inf::globalNodeCount(mp, *mesh));
        if(mp.Rank() == 0) {
            FILE* f = fopen(filename.c_str(), "w");
            int n_meshes = int(meshes.size());
            fprintf(f, "%i\n", n_meshes);
            for (int i = 0; i < n_meshes - 1; i++) fprintf(f, "%li %i\n", node_counts[i], i + 1);
            fprintf(f, "%li 0\n", node_counts.back());
            fclose(f);
        }
    }

  private:


    void prependBoundaryConditionsWithBodyName(Parfait::BoundaryConditionMap& bc_map,const std::string& body_name,RootPrinter rp) const {
        for(auto& bc:bc_map) {
            auto& name = bc.second.second;
            int bc_number = bc.second.first;
            if(not body_name.empty()) {
                if (isFun3DSolidWallBc(bc_number)) {
                    auto rename = body_name;
                    rp.print("Renaming: " + name + " ---> " + rename + "\n");
                    name = rename;
                } else {
                    auto rename = body_name + "_" + name;
                    rp.print("Renaming: " + name + " ---> " + rename + "\n");
                    name = rename;
                }
            }
        }
    }

    bool isFun3DSolidWallBc(int bc_number) const {
        switch(bc_number){
            case 3000:
            case 4000:
                return true;
            default:
                return false;
        }
    }

    void readAndCombineMapbcFiles(const std::vector<MotionParser::GridEntry>& motion_entries,
                                  const std::string& composite_grid_base,
                                  bool should_lump){
        RootPrinter rp(0);
        std::vector<Parfait::BoundaryConditionMap> bc_maps;
        std::vector<std::string> bc_map_files;
        for (auto& entry:motion_entries) {
            rp.print("Importing mapbc file: "+entry.mapbc_name+"\n");
            Parfait::MapbcReader reader(entry.mapbc_name);
            if(reader.readFailed()){
                throw std::logic_error("Failed to read mapbc file: "+entry.mapbc_name);
            }
            auto bc = reader.getMap();
            prependBoundaryConditionsWithBodyName(bc, entry.domain_name, rp);
            if(should_lump){
                auto old_to_new = groupTagsByBoundaryConditionName(bc);
                bc = lumpBC(bc,old_to_new);
            }
            bc_maps.push_back(bc);
        }
        auto composite_mapbc = inf::MapBcCombiner::combine(bc_maps);
        auto out_name = composite_grid_base + ".mapbc";
        rp.print("Writing: "+out_name+"\n");
        MapBcCombiner::write(out_name, composite_mapbc);
    }
};
}

CREATE_INF_SUBCOMMAND(inf::MakeCompositeCommand)