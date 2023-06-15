#pragma once
#include <t-infinity/MotionMatrixParser.h>
#include <parfait/FileTools.h>
#include <parfait/LinearPartitioner.h>
#include <t-infinity/DivineLoadBalancer.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/MeshMover.h>
#include "RootPrinter.h"

namespace YOGA {

class MeshSystemLoader{
  public:
    MeshSystemLoader(Parfait::CommandLineMenu& m,MessagePasser global_mp)
        :m(m), grid_mp(global_mp){

        RootPrinter groot(global_mp.Rank());
        using namespace inf;
        auto script_filename = m.get("file");
        groot.print("Reading assembly info from: "+script_filename +"\n");
        auto script = Parfait::FileTools::loadFileToString(script_filename);
        auto bc_filename = m.get("bc");
        groot.print("Reading boundary conditions from: "+bc_filename+"\n");
        bc_string = Parfait::FileTools::loadFileToString(bc_filename);
        auto grid_recipes = MotionParser::parse(script);
        if(global_mp.NumberOfProcesses() < int(grid_recipes.size()))
          throw std::logic_error("Need at least as many ranks ("+std::to_string(global_mp.NumberOfProcesses())+") as components ("+std::to_string(grid_recipes.size())+")");

        DivineLoadBalancer balancer(global_mp.Rank(),global_mp.NumberOfProcesses());
        auto grid_weights = estimateGridWeightsBasedOnFileSize(grid_recipes);
        grid_id = balancer.getAssignedDomain(grid_weights);

        auto grid_comm = global_mp.split(global_mp.getCommunicator(),grid_id);
        grid_mp = MessagePasser(grid_comm);

        RootPrinter root(grid_mp.Rank());
        root.print("Loading [grid "+std::to_string(grid_id)+"] "+grid_recipes[grid_id].grid_name+" on "+std::to_string(grid_mp.NumberOfProcesses())+" ranks\n");
        grid_name = grid_recipes[grid_id].grid_name;
        auto preprocessor_name = pickPreProcessorByExtension(grid_name);
        mesh_ptr = shortcut::loadMesh(grid_mp,grid_name, preprocessor_name);
        motion = grid_recipes[grid_id].motion_matrix;
        mesh_ptr = inf::MeshMover::move(mesh_ptr,motion);
    }

    MessagePasser gridComm(){return grid_mp;}
    std::shared_ptr<inf::MeshInterface> mesh(){return mesh_ptr;}
    int gridId(){return grid_id;}
    std::string bcString(){return bc_string;}
    Parfait::MotionMatrix gridMotion(){return motion;}

    std::vector<double> estimateGridWeightsBasedOnFileSize(
        std::vector<inf::MotionParser::GridEntry> grid_recipes) {
        int n = int(grid_recipes.size());
        std::vector<double> weights(n);
        for (int i = 0; i < n; i++) weights[i] = calcFileSize(grid_recipes[i].grid_name);
        normalize(weights);
        return weights;
    }
  private:
    Parfait::CommandLineMenu m;
    MessagePasser grid_mp;
    std::shared_ptr<inf::MeshInterface> mesh_ptr;
    std::string grid_name;
    std::string bc_string;
    Parfait::MotionMatrix motion;
    int grid_id;

    std::string pickPreProcessorByExtension(const std::string& name){
        auto extension = Parfait::StringTools::getExtension(name);
        if("meshb" == extension)
            return "RefinePlugins";
        return "NC_PreProcessor";
    }

    size_t calcFileSize(const std::string& filename) {
        size_t bytes = 0;
        FILE* f = fopen(filename.c_str(), "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            bytes = ftell(f);
            fclose(f);
        } else {
            throw std::logic_error("Could not open grid file: " + filename);
        }
      return bytes;
  }
    void normalize(std::vector<double>& v) {
        double max = 0;
        for(auto d:v)
            max = std::max(max,std::abs(d));
        double x = 1.0/max;
        for(auto& d:v)
            d *= x;
  }
};




}
