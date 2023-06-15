#include <stdio.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <parfait/LinePlotters.h>

using namespace inf;
using namespace Parfait;
using namespace std;

class SplitCsvCommand : public SubCommand {
  public:
    std::string description() const override { return "Split composite csv based on imesh"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(), Help::help());
        m.addParameter(Alias::inputFile(),Help::inputFile(),true);
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "component_");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        if(m.has(Alias::inputFile()) and m.has(Alias::outputFileBase())){
            auto fun3d_output = m.get(Alias::inputFile());
            auto basename = m.get(Alias::outputFileBase());
            auto data_frame = LinePlotters::CSVReader::read(mp, fun3d_output);
            auto sub_data_frame_map = data_frame.groupBy("imesh");
            std::set<int> imesh_values;
            for(auto&pair:sub_data_frame_map)
                imesh_values.insert(pair.first);
            imesh_values = mp.ParallelUnion(imesh_values);
            std::map<std::string,std::vector<double>> mapped_columns;
            for(auto name:data_frame.columns())
                mapped_columns[name] = {};
            Parfait::DataFrame emptySubFrame(mapped_columns);
            for(int imesh:imesh_values){
                auto output_filename = basename + to_string(imesh) + ".csv";
                auto frame = emptySubFrame;
                if(sub_data_frame_map.count(imesh) == 1)
                    frame = sub_data_frame_map[imesh];
                LinePlotters::CSVWriter::write(mp, output_filename, frame);
            }

        }
        else{
            printf("%s\n", m.to_string().c_str());
        }
    }

  private:

};

CREATE_INF_SUBCOMMAND(SplitCsvCommand)
