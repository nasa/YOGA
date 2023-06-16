#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <RotorInputParser.h>
#include <MovingBodyParser.h>
#include <parfait/FileTools.h>
#include <RotorPlacer.h>

using namespace inf;
using namespace YOGA;
class CompositeRotorCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Generate composite builder script from FUN3D rotor.input and moving_body.input"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter({"o"}, "Output filename", false, "composite");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto rotors = extractRotorInput("rotor.input");
        auto moving_bodies = extractMovingBodyInput("moving_body.input");

        int total_blades = countRotorBlades(rotors);
        int nbodies = moving_bodies.size();
        if(total_blades > nbodies){
            printf("rotor.input specifies %i blades,\n",total_blades);
            printf("but moving_body.input only specifies %i moving bodies.\n",nbodies);
            return;
        }

        int n_rotors = rotors.size();
        printf("Number of rotors: %i\n",n_rotors);
        std::vector<std::string> grid_filenames;
        std::vector<std::string> mapbc_filenames;
        std::string grid = "rotor.b8.ugrid";

        for(int i=0;i<n_rotors;i++){
            grid = askForGridFileName(grid, i, rotors[i].clockwise);
            auto mapbc = askForMapbcFileName(grid);
            grid_filenames.push_back(grid);
            mapbc_filenames.push_back(mapbc);
        }
        grid = askForBackgroundGridFileName();
        grid_filenames.push_back(grid);
        auto mapbc = askForMapbcFileName(grid);
        mapbc_filenames.push_back(mapbc);

        auto outfile = m.get(Alias::outputFileBase()) + ".txt";
        writeCompositeScript(outfile, rotors, moving_bodies, grid_filenames,mapbc_filenames);
    }

  private:

    std::vector<RotorInputParser::RotorConfiguration> extractRotorInput(const std::string& filename) const {
        if(Parfait::FileTools::doesFileExist(filename)){
            printf("Reading: %s\n",filename.c_str());
            auto s = Parfait::FileTools::loadFileToString(filename);
            return RotorInputParser::parse(s);
        }
        else{
            printf("File not found: %s\n",filename.c_str());
            return {};
        }
    }

    int countRotorBlades(const std::vector<RotorInputParser::RotorConfiguration>& rotors){
        int n = 0;
        for(auto r:rotors)
            n += r.blade_count;
        return n;
    }

    std::vector<MovingBodyInputParser::Body> extractMovingBodyInput(const std::string& filename) const {
        if(Parfait::FileTools::doesFileExist(filename)){
            printf("Reading: %s\n",filename.c_str());
            auto s = Parfait::FileTools::loadFileToString(filename);
            return MovingBodyInputParser::parse(s);
        }
        else{
            printf("File not found: %s\n",filename.c_str());
            return {};
        }
    }

    std::string askForBackgroundGridFileName(){
        std::string s;
        printf("Enter background grid filename: ");
        std::cin >> s;
        return s;
    }

    std::string askForGridFileName(const std::string& previous_grid_filename, int n, bool clockwise){
        std::string s;
        std::string orientation = clockwise ? "clockwise" : "counter-clockwise";
        printf("Enter blade grid name for <rotor-%i> <%s> (or '.' to use %s): ",n,
               orientation.c_str(),
               previous_grid_filename.c_str());
        std::cin >> s;
        if("." == s)
            return previous_grid_filename;
        return s;
    }

    std::string askForMapbcFileName(const std::string& grid_name){
        auto mapbc = suggestMapbcNameFromUgrid(grid_name);
        printf("Enter mapbc name (or '.' to use %s): ",mapbc.c_str());
        std::string s;
        std::cin >> s;
        if("." == s)
            return mapbc;
        return s;
    }

    std::string suggestMapbcNameFromUgrid(const std::string& grid){
        std::string mapbc = Parfait::StringTools::stripExtension(grid,{"lb8.ugrid","b8.ugrid","ugrid"});
        mapbc.append(".mapbc");
        return mapbc;
    }

    void writeCompositeScript(const std::string& outfile,
                              const std::vector<RotorInputParser::RotorConfiguration>& rotors,
                              const std::vector<MovingBodyInputParser::Body>& bodies,
                              const std::vector<std::string>& grid_files,
                              const std::vector<std::string>& mapbc_files) const {
        int n_rotors = rotors.size();
        int current_body = 0;
        FILE* f = fopen(outfile.c_str(),"w");
        for(int i=0;i<n_rotors;i++){
            auto& rotor = rotors[i];
            auto& grid = grid_files[i];
            auto& mapbc = mapbc_files[i];
            auto placer = getRotorPlacer(rotor);
            for(int j=0;j<rotor.blade_count;j++){
                auto body_name = bodies[current_body++].name;
                auto motion = placer.getBladeMotion(j);
                auto blade_string = formatBlade(placer,grid,mapbc,body_name,motion);
                fprintf(f,"%s\n",blade_string.c_str());
            }
        }
        fprintf(f,"#---- Background grid -----------------\n");
        fprintf(f,"grid %s\n",grid_files.back().c_str());
        fprintf(f,"mapbc %s\n",mapbc_files.back().c_str());

        fclose(f);
    }

    std::string formatBlade(const RotorPlacer& placer,
                            const std::string& grid,
                            const std::string& mapbc,
                            const std::string& body_name,
                            const Parfait::MotionMatrix& motion) const {
        std::string s;
        s += "grid " + grid + "\n";
        s += "mapbc " + mapbc + "\n";
        s += "domain " + body_name + "\n";
        s += "move\n" + motion.toString();
        s += "\n";
        return s;
    }

    RotorPlacer getRotorPlacer(const RotorInputParser::RotorConfiguration& rotor) const {
        bool is_clockwise = not rotor.clockwise;
        RotorPlacer rotor_placer(rotor.blade_count, is_clockwise);
        rotor_placer.translate(rotor.translation);
        rotor_placer.roll(rotor.roll);
        rotor_placer.pitch(rotor.pitch);
        rotor_placer.yaw(rotor.yaw);
        return rotor_placer;
    }

};

CREATE_INF_SUBCOMMAND(CompositeRotorCommand)
