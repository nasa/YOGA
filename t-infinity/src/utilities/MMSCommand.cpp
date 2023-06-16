#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/Shortcuts.h>

using namespace inf;

class MMSCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Metric operations"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(), Help::help());
        m.addParameter(Alias::mesh(),Help::mesh(),false);
        m.addParameter(Alias::outputFileBase(),"output filename", false);
        m.addParameter({"function"}, "number of selected function", false);
        m.addFlag({"list-functions"},"list available mms functions");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        if(m.has("list-functions")){
            printFunctionList(mp);
            return;
        }

        if(m.has(Alias::mesh()) and m.has("function")) {
            auto meshfile = m.get(Alias::mesh());
            auto mesh_loader = selectDefaultPreProcessorFromMeshFilename(meshfile);
            auto outfile = m.get(Alias::outputFileBase());
            auto mesh = shortcut::loadMesh(mp, meshfile, mesh_loader);

            auto field = getFieldFromSelectedFunction(m.getInt("function"), *mesh);

            Snap snap(mp.getCommunicator());
            snap.addMeshTopologies(*mesh);
            snap.add(field);
            snap.writeFile(outfile);
        }
    }

  private:
    void printFunctionList(MessagePasser mp){
        if(mp.Rank() > 0) return;
        printf("Available functions:\n");
        printf("    1:    sin(x)*cos(y)*sin(xz)\n");
        printf("    2:    cos(x)*sin(y)*sin(xz)\n");
    }

    double selectedFunction(int function_n, const Parfait::Point<double>& p){
        double x=p[0],y=p[1],z=p[2];
        switch (function_n) {
            case 1: return sin(x)*cos(y)*sin(x*z);
            case 2: return cos(x)*sin(y)*sin(x*z);
            default: return 0.0;
        }
    }

    std::shared_ptr<FieldInterface> getFieldFromSelectedFunction(int function_n, const MeshInterface& mesh){
        std::vector<double> scalar(mesh.nodeCount(), 0.0);
        for(int i=0;i<mesh.nodeCount();i++) {
            Parfait::Point<double> p;
            mesh.nodeCoordinate(i,p.data());
            scalar[i] = selectedFunction(function_n, p);
        }
        return std::make_shared<VectorFieldAdapter>("function_"+std::to_string(function_n), FieldAttributes::Node(), scalar);
    }
};


CREATE_INF_SUBCOMMAND(MMSCommand)
