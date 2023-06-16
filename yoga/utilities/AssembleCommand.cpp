#include <t-infinity/SubCommand.h>
#include "AssemblyHelpers.h"
#include <parfait/Timing.h>
#include <parfait/Dictionary.h>
#include "DistanceCalculator.h"
#include "BoundaryConditionParser.h"
#include "YogaPlugin.h"
#include "YogaConfiguration.h"
#include "DcifWriter.h"
#include <t-infinity/FieldTools.h>
#include "GlobalIdShifter.h"
#include "Diagnostics.h"
#include <t-infinity/Extract.h>
#include <t-infinity/PointCloudInterpolator.h>
#include <t-infinity/MeshConnectivity.h>
#include <parfait/PointwiseSources.h>
#include <parfait/Flatten.h>
#include <parfait/Checkpoint.h>
#include <t-infinity/VisualizePartition.h>
using namespace YOGA;
using namespace inf;

class AssembleCommand : public inf::SubCommand{
  public:
    std::string description() const override { return "dry run off domain assembly without flow solver"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter({"file"},"composite mesh input file",true);
        m.addParameter({"bc"},"boundary condition file",true);
        m.addParameter({"viz"},"export assembly to file based on extension",false);
        m.addParameter({"plot"},"options [orphans]",false);
        m.addParameter({"part"},"dump FUN3D partition to filename",false);
        m.addFlag({"check-donorGids"},"make sure interpolation recovers a linear field");
        m.addFlag({"d","debug"}, "output diagnostics file for debugging");
        return m;
    }

    struct TypeCounts{
        long in = 0;
        long out = 0;
        long receptor = 0;
        long orphan = 0;
    };

    void run(Parfait::CommandLineMenu m, MessagePasser global_mp) override {
        MeshSystemLoader loader(m,global_mp);
        MessagePasser mp(loader.gridComm());
        RootPrinter root(global_mp.Rank());
        auto mesh = loader.mesh();
        int grid_id = loader.gridId();
        if(m.has("part")) {
            auto filename = m.get("part");
            auto gids = extractOwnedGIDS(global_mp, mp, *mesh, grid_id);
            bool swap_bytes = true;
            writeFUN3DPartitionFile(global_mp,filename,gids,swap_bytes);
            return;
        }
        auto config = YOGA::YogaConfiguration(mp);
        auto ranks_to_trace = config.getRanksToTrace();
        auto basename = config.getTracerBaseName();
        Tracer::initialize(basename,global_mp.Rank(),ranks_to_trace);
        Tracer::setDebug();
        global_mp.Barrier();
        Tracer::begin("load yoga plugin");
        YogaPlugin yoga(global_mp,*mesh,grid_id,loader.bcString());
        Tracer::end("load yoga plugin");


        global_mp.Barrier();
        Tracer::begin("perform assembly");
        auto keepers = yoga.performAssembly();
        Tracer::end("perform assembly");

        auto node_statuses = yoga.field("node statuses");
        auto cell_statuses = yoga.field("cell statuses");

        if(m.has("viz")) {
            auto filename = m.get("viz");
            auto extension = Parfait::StringTools::getExtension(filename);
            if("dcif" == extension){
                exportToDcif(global_mp,filename,yoga);
            }
            else{
                filename.resize(filename.size() - extension.size() - 1);
                filename += "_" + std::to_string(grid_id) + "." + extension;
                inf::shortcut::visualize(
                    filename, mp, mesh, {node_statuses, cell_statuses});
            }
        }

        if(m.has("plot")){
            auto options = m.getAsVector("plot");
            if(std::count(options.begin(),options.end(),"orphans")){
                auto cell_status_vector = FieldTools::extractColumn(*cell_statuses,0);
                auto selector = std::make_shared<CellValueSelector>(NodeStatus::Orphan,NodeStatus::Orphan,cell_status_vector);
                auto spacing_at_nodes = generateSpacingAtNodes(*mesh);
                CellIdFilter orphan_filter(mp.getCommunicator(),mesh,selector);
                auto orphan_mesh = orphan_filter.getMesh();
                inf::Interpolator pointCloudInterpolator(global_mp,mesh,orphan_mesh);
                auto spacing_field = std::make_shared<VectorFieldAdapter>("spacing",FieldAttributes::Node(),spacing_at_nodes);
                auto interpolated_spacing = pointCloudInterpolator.interpolate(spacing_field);

                auto orphan_xyz = inf::extractOwnedPoints(*orphan_mesh);
                std::vector<double> orphan_spacing;
                std::vector<double> orphan_decay(orphan_xyz.size(),0.5);
                for(int i=0;i<orphan_mesh->nodeCount();i++) {
                    if(orphan_mesh->ownedNode(i)){
                        double dx;
                        interpolated_spacing->value(i, &dx);
                        if(dx < 1e-10){
                            int node_id = orphan_mesh->previousNodeId(i);
                            dx = spacing_at_nodes[node_id];
                        }
                        orphan_spacing.push_back(dx);
                    }
                }
                if(orphan_spacing.size() != orphan_xyz.size())
                    throw std::logic_error("Orphan spacing not found for each xyz");

                // gather to root and export
                auto all_xyz = global_mp.Gather(orphan_xyz,0);
                auto all_spacing = global_mp.Gather(orphan_spacing,0);
                auto all_decay = global_mp.Gather(orphan_decay,0);

                if(0 == global_mp.Rank()){
                    auto xyz = Parfait::flatten(all_xyz);
                    auto spacing = Parfait::flatten(all_spacing);
                    auto decay = Parfait::flatten(all_decay);
                    Parfait::PointwiseSources pw_sources(xyz,spacing,decay);
                    pw_sources.write("orphan_spacing_update.pcd");
                }

                std::string filename = "orphans_"+std::to_string(grid_id)+".vtk";
                root.print("Exporting: "+filename+"\n");
                inf::shortcut::visualize(filename,mp,orphan_mesh);
            }
        }

        if(m.has("debug")){
            auto type_counts = countTypes(global_mp, *node_statuses, *mesh);
            Parfait::Dictionary dict;
            dict["node-statuses"]["receptor"] = int(type_counts.receptor);
            dict["node-statuses"]["in"] = int(type_counts.in);
            dict["node-statuses"]["out"] = int(type_counts.out);
            dict["node-statuses"]["orphan"] = int(type_counts.orphan);
            Parfait::FileTools::writeStringToFile("yoga_debug.json", dict.dump());
        }

        if(m.has("check-donorGids")){
            yoga.verifyDonors();
        }
    }

  private:

    std::vector<double> generateSpacingAtNodes(const inf::MeshInterface& mesh){
        std::vector<double> spacing(mesh.nodeCount());
        auto n2n = inf::NodeToNode::build(mesh);
        for(int node_id=0;node_id<mesh.nodeCount();node_id++){
            Parfait::Point<double> p;
            mesh.nodeCoordinate(node_id,p.data());
            double min_edge_length = std::numeric_limits<double>::max();
            for(auto nbr:n2n[node_id]){
                Parfait::Point<double> p2;
                mesh.nodeCoordinate(nbr,p2.data());
                double edge_length = Parfait::Point<double>::distance(p,p2);
                min_edge_length = std::min(edge_length, min_edge_length);
            }
            spacing[node_id] = min_edge_length;
        }
        return spacing;
    }



    std::vector<long> extractOwnedGIDS(MessagePasser global_mp,MessagePasser component_grid_mp, inf::MeshInterface& mesh, int grid_id){
        auto owned_gids = inf::extractOwnedGlobalNodeIds(mesh);
        long offset = GlobalIdShifter::calcGlobalIdOffsetForComponentGrid(global_mp,owned_gids,grid_id);
        auto biggest = global_mp.ParallelMax(offset,0);
        if(0 == global_mp.Rank())
            printf("Biggest offset: %li\n",biggest);
        for(long& gid:owned_gids)
            gid += offset;
        return owned_gids;
    }

    TypeCounts countTypes(MessagePasser mp, const inf::FieldInterface& node_statuses, const inf::MeshInterface& mesh){
        TypeCounts counts;
        for(int i=0;i<mesh.nodeCount();i++){
            if(mesh.ownedNode(i)) {
                double status;
                node_statuses.value(i,&status);
                if(status == NodeStatus::InNode)
                    counts.in++;
                else if(status == NodeStatus::OutNode)
                    counts.out++;
                else if(status == NodeStatus::FringeNode)
                    counts.receptor++;
                else if(status == NodeStatus::Orphan)
                    counts.orphan++;
            }
        }

        counts.in = mp.ParallelSum(counts.in);
        counts.out = mp.ParallelSum(counts.out);
        counts.receptor = mp.ParallelSum(counts.receptor);
        counts.orphan = mp.ParallelSum(counts.orphan);

        return counts;
    }

    void exportToDcif(MessagePasser mp,const std::string& filename, YogaPlugin& yoga){
        if(mp.Rank()==0)
            printf("Exporting: %s\n",filename.c_str());
        DcifWriter dcif_writer(mp);

        DomainConnectivityInfo<double> dci(mp, yoga.mesh, yoga.node_statuses, yoga.receptors);
        dcif_writer.exportDcif(filename,yoga.mesh, dci);
    }
};

CREATE_INF_SUBCOMMAND(AssembleCommand)