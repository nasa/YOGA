#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <parfait/LinePlotters.h>
#include <t-infinity/Snap.h>
#include <t-infinity/VectorFieldAdapter.h>

namespace inf  {

class CsvToSnapCommand : public SubCommand {
  public:
    std::string description() const override { return "convert csv format --> snap"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::inputFile(), "input csv file(s)", true);
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "out");
        m.addParameter({"only"},"list fields to select",false);
        m.addParameter({"at"},"cell or node",false, "node");
        m.addParameter({"--delimiter", "-d"}, "csv delimiter", false, ",");
        return m;
    }

    std::string association(Parfait::CommandLineMenu m){
        auto requested =  m.get("at");
        requested = Parfait::StringTools::tolower(requested);
        if(Parfait::StringTools::startsWith(requested, "cell"))
            return inf::FieldAttributes::Cell();
        if(Parfait::StringTools::startsWith(requested, "node"))
            return inf::FieldAttributes::Node();
        PARFAIT_THROW("Unknown field association requested!" + m.get("at") + "\nValid Options are cell or node");
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto filenames = m.getAsVector(Alias::inputFile());
        auto output_name = m.get(Alias::outputFileBase());

        for (const auto& filename : filenames) {
            printf("Importing: %s\n", filename.c_str());
            auto csv = Parfait::LinePlotters::CSVReader::read(mp,filename,m.get("--delimiter"));

            long count = csv.shape()[0];
            auto count_per_rank = mp.Gather(count);
            long offset = 0;
            for(int i=0;i<mp.Rank();i++)
                offset += count_per_rank[i];

            std::vector<long> global_ids(count);
            std::iota(global_ids.begin(), global_ids.end(), offset);
            std::vector<bool> do_own(count, true);

            inf::Snap snap(mp.getCommunicator());
            snap.setTopology(inf::FieldAttributes::Node(), global_ids, do_own);
            auto field_names = csv.columns();
            if(m.has("only"))
                field_names = m.getAsVector("only");
            for (auto& field_name : field_names) {
                if(0 == mp.Rank()) printf("Adding field: %s\n", field_name.c_str());
                auto field = std::make_shared<inf::VectorFieldAdapter>(
                    field_name, association(m), csv[field_name]);
                snap.add(field);
            }
            if (filenames.size() > 1) output_name = Parfait::StringTools::stripExtension(filename, ".csv") + ".snap";
            if(0 == mp.Rank()) printf("Writing: %s\n", output_name.c_str());
            snap.writeFile(output_name);
        }
    }
};
}

CREATE_INF_SUBCOMMAND(inf::CsvToSnapCommand)