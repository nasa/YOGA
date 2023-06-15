#include <stdio.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include "AssemblyHelpers.h"
#include <IsotropicSpacingTree.h>
#include <parfait/PointwiseSources.h>
#include <t-infinity/Extract.h>
#include <t-infinity/MetricManipulator.h>
#include "BoundaryConditionParser.h"
#include <t-infinity/Snap.h>

using namespace YOGA;

class FixOrphanCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Adaptation experiments"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(inf::Alias::help(), inf::Help::help());
        m.addParameter({"file"},"composite mesh input file",true);
        m.addParameter({"bc"},"boundary condition file",true);
        m.addParameter({"steps"},"number of steps to rotate",false, "1");
        m.addParameter({"degrees"}, "degrees to rotate per step", false, "0");
        m.addParameter({"axis"},"axis of rotation, prior to grid motion/placement", false, "0,0,1");
        m.addParameter({"components"},"list of component grid ids to rotate e.g., 3:5,8,12",false,"");
        m.addParameter({"min-spacing"},"spacing floor, e.g., use 0.1 if tip-chord is 1.0 to get 10% tip-chord-spacing",false,"0.5");
        m.addParameter({"o"},"output filename for sources/metric field (*.pcd / *.snap)","sources.pcd");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser global_mp) override {
        MeshSystemLoader loader(m,global_mp);
        //MessagePasser mp(loader.gridComm());
        RootPrinter root(global_mp.Rank());
        auto mesh = loader.mesh();
        int grid_id = loader.gridId();
        auto motion = loader.gridMotion();
        Parfait::Point<double> rotor_shaft_begin {0,0,0};
        double min_spacing = m.getDouble("min-spacing");
        auto axis = Parfait::StringTools::split(m.get("axis"),",");
        double nx = Parfait::StringTools::toDouble(axis[0]);
        double ny = Parfait::StringTools::toDouble(axis[1]);
        double nz = Parfait::StringTools::toDouble(axis[2]);
        Parfait::Point<double> rotor_shaft_end {nx,ny,nz};
        auto moving_component_ids = Parfait::StringTools::parseIntegerList(m.get("components"));
        bool is_moving = std::count(moving_component_ids.begin(),moving_component_ids.end(),grid_id);
        auto all_bcs = BoundaryInfoParser::parse(loader.bcString());
        auto my_bcs = all_bcs[grid_id];
        std::set<int> my_interp_tags(my_bcs.interpolation_tags.begin(),my_bcs.interpolation_tags.end());
        auto tree = generateSpacingTreeOnRoot(*mesh, global_mp,my_interp_tags,min_spacing);
        int steps = m.getInt("steps");
        motion.movePoint(rotor_shaft_begin.data());
        motion.movePoint(rotor_shaft_end.data());
        double angle = m.getDouble("degrees");
        Parfait::MotionMatrix rotate;
        rotate.addRotation(rotor_shaft_begin,rotor_shaft_end,angle);
        double degrees_so_far = 0.0;
        for(int i=0;i<steps;i++){
            root.println("Angle: "+std::to_string(degrees_so_far));
            if(is_moving)
                mesh = inf::MeshMover::move(mesh, rotate);
            updateSpacingTreeOnRoot(tree,*mesh,global_mp,my_interp_tags);
            degrees_so_far += angle;
        }
        auto outfile = m.get("o");
        auto extension = Parfait::StringTools::getExtension(outfile);
        if("pcd" == extension) {
            if (global_mp.Rank() == 0) {
                tree->visualize("boxes.vtk");
                auto spacing = tree->spacingValues();
                std::vector<Parfait::Point<double>> points;
                std::vector<double> h;
                for (auto& s : spacing) {
                    points.push_back(s.p);
                    h.push_back(s.h);
                }
                std::vector<double> decay(points.size(), 0.5);
                Parfait::PointwiseSources sources(points, h, decay);
                sources.write(outfile);
            }
        }
        else if("snap" == extension){
            auto component_mp = loader.gridComm();
            auto all_grid_ids = global_mp.ParallelUnion(std::set<int>{grid_id});
            int id_of_background_grid = 0;
            int n_background_grids = 0;
            for(int gid:all_grid_ids){
                if(std::count(moving_component_ids.begin(),moving_component_ids.end(),gid) == 0) {
                    id_of_background_grid = gid;
                    n_background_grids++;
                }
            }
            if(1 == n_background_grids){
                exportAsMetric(global_mp, component_mp, grid_id, id_of_background_grid, *tree, *mesh, outfile);
            }
            else{
                root.print("Expected 1 background grid, found "+std::to_string(n_background_grids));
            }

        }
    }

  private:

    void updateSpacingTreeOnRoot(std::shared_ptr<IsotropicSpacingTree> tree,
                                 const inf::MeshInterface& mesh, MessagePasser global_mp, const std::set<int>& interp_tags){
        auto spacing_values = generateSpacingValues(mesh,interp_tags);

        global_mp.Barrier();
        if(0 == global_mp.Rank()) {
            for(auto& s:spacing_values){
                Parfait::Extent<double> item_extent {s.p, s.p};
                tree->insert(item_extent, s.h);
            }
        }

        global_mp.Barrier();
        for(int i=1;i<global_mp.NumberOfProcesses();i++){
            if(global_mp.Rank() == i)
                global_mp.Send(spacing_values, 0);
            if(global_mp.Rank() == 0){
                std::vector<IsotropicSpacingTree::Spacing> spacing_from_rank;
                global_mp.Recv(spacing_from_rank, i);
                for(auto& s:spacing_from_rank){
                    Parfait::Extent<double> item_extent {s.p, s.p};
                    tree->insert(item_extent, s.h);
                }
            }
            global_mp.Barrier();
        }
    }

    std::shared_ptr<IsotropicSpacingTree> generateSpacingTreeOnRoot(const inf::MeshInterface& mesh,
                                                                    MessagePasser global_mp,
                                                                    const std::set<int>& interpolation_tags,
                                                                    double min_spacing){
        global_mp.Barrier();
        using namespace Parfait::ExtentBuilder;
        auto partition_extent = buildExtentForPointsInMesh(mesh);
        auto all_partition_extents = global_mp.Gather(partition_extent,0);
        std::shared_ptr<IsotropicSpacingTree> spacing_tree = nullptr;
        if(global_mp.Rank() == 0) {
            auto domain = Parfait::ExtentBuilder::getBoundingBox(all_partition_extents);
            spacing_tree = std::make_shared<IsotropicSpacingTree>(domain);
            spacing_tree->setMinSpacing(min_spacing);
        }
        updateSpacingTreeOnRoot(spacing_tree,mesh,global_mp,interpolation_tags);
        return spacing_tree;
    }

    std::vector<IsotropicSpacingTree::Spacing> generateSpacingValues(const inf::MeshInterface& mesh,const std::set<int>& interpolation_tags){
        std::vector<IsotropicSpacingTree::Spacing> spacing;
        for(int cell=0;cell<mesh.cellCount();cell++)
            if(interpolation_tags.count(mesh.cellTag(cell)) == 1)
            spacing.push_back(generateSpacingForCell(mesh,cell));
        return spacing;
    }

    IsotropicSpacingTree::Spacing generateSpacingForCell(const inf::MeshInterface& mesh, int cell_id){
        auto e = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
        std::vector<int> cell;
        mesh.cell(cell_id,cell);
        for (int node_id : cell) {
            Parfait::Point<double> p;
            mesh.nodeCoordinate(node_id,p.data());
            Parfait::ExtentBuilder::add(e, p);
        }
        std::array<double,3> lengths;
        lengths[0] = e.getLength_X();
        lengths[1] = e.getLength_Y();
        lengths[2] = e.getLength_Z();
        std::sort(lengths.begin(),lengths.end());
        double h = lengths[1];
        return IsotropicSpacingTree::Spacing(e.center(),h);
    }

    void exportAsMetric(MessagePasser global_mp,
                        MessagePasser component_mp,
                        int component_id,
                        int background_component_id,
                        const IsotropicSpacingTree& tree,
                        const inf::MeshInterface& mesh,
                        const std::string& outfile){
        if(0 == global_mp.Rank()) printf("Extract spacing for points in mesh\n");
        auto spacing_for_points = getIsotropicSpacingAtPoints(global_mp,tree,mesh);
        if(0 == global_mp.Rank()) printf("Convert to metric field\n");
        std::vector<inf::Tensor> metric(mesh.nodeCount(),inf::Tensor::Identity());
        for(int i=0;i<mesh.nodeCount();i++){
            auto& m = metric[i];
            double h = spacing_for_points[i];
            double eigenvalue = 1.0/(h*h);
            auto decomp = Parfait::MetricDecomposition::decompose(m);
            decomp.D(0,0) = eigenvalue;
            decomp.D(1,1) = eigenvalue;
            decomp.D(2,2) = eigenvalue;
            m = Parfait::MetricDecomposition::recompose(decomp);
        }
        if(0 == global_mp.Rank()) printf("Export metric for component <%i> to: %s\n",background_component_id,outfile.c_str());
        if(background_component_id == component_id) {
            if(int(metric.size()) != mesh.nodeCount()){
                throw std::logic_error("Entries in metric != nodeCount "+std::to_string(metric.size())+" != "+std::to_string(mesh.nodeCount()));
            }
            auto field = inf::MetricManipulator::toNodeField(metric);
            inf::Snap snap(component_mp.getCommunicator());
            snap.addMeshTopologies(mesh);
            snap.add(field);
            snap.writeFile(outfile);
        }
    }

    std::vector<double> getIsotropicSpacingAtPoints(MessagePasser mp,const IsotropicSpacingTree& tree,
        const inf::MeshInterface& mesh){
        auto points = inf::extractPoints(mesh);
        std::vector<double> spacings_for_points(points.size());
        for(int rank=1;rank<mp.NumberOfProcesses();rank++){
            if(rank == mp.Rank()){
                mp.Send(points,0);
                mp.Recv(spacings_for_points,0);
            }
            else if(0 == mp.Rank()){
                printf("Getting isotropic spacing for rank %i\n",rank);
                std::vector<Parfait::Point<double>> points_from_rank;
                mp.Recv(points_from_rank,rank);
                std::vector<double> spacings_for_rank(points_from_rank.size());
                for(int i=0;i<int(points_from_rank.size());i++)
                    spacings_for_rank[i] = tree.getSpacingAt(points_from_rank[i]);
                mp.Send(spacings_for_rank,rank);
            }
            mp.Barrier();
        }
        if(0 == mp.Rank()){
            for(int i=0;i<mesh.nodeCount();i++)
                spacings_for_points[i] = tree.getSpacingAt(points[i]);
        }
        return spacings_for_points;
    }

};

CREATE_INF_SUBCOMMAND(FixOrphanCommand)
