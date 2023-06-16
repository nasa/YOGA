#include <stdio.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include "DcifReader.h"
#include "DcifChecker.h"
#include <parfait/UgridReader.h>

using namespace inf;
using namespace YOGA;

class CheckDcifCommand : public SubCommand {
  public:
    std::string description() const override { return "Check fun3d-dcif file for consistency"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(), Help::help());
        m.addParameter(Alias::inputFile(), Help::inputFile(), true);
        m.addParameter(Alias::mesh(),Help::mesh(),false);
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto filename = m.get(inf::Alias::inputFile());
        DcifReader reader(filename);

        auto iblank = reader.getIblank();
        auto receptor_ids = reader.getFringeIds();
        DcifChecker::throwIfCountsMismatch(iblank,receptor_ids);
        DcifChecker::throwIfBadReceptorIdFound(iblank,receptor_ids);
        DcifChecker::throwIfReceptorsAreInconsistent(iblank,receptor_ids);

        if(m.has(Alias::mesh()) and mp.Rank() == 0) {
            Parfait::UgridReader ugrid_reader(m.get(Alias::mesh()));
            auto raw_vertex_data = ugrid_reader.readNodes();
            int nnodes = raw_vertex_data.size() / 3;
            std::vector<Parfait::Point<double>> points(nnodes);
            for(int i=0;i<nnodes;i++)
                for(int j=0;j<3;j++)
                    points[i][j] = raw_vertex_data[3*i+j];

            FILE* f = fopen("composite-gids","w");
            for(int i=0;i<nnodes;i++){
                fprintf(f,"%i %s\n",i,points[i].to_string().c_str());
            }
            fclose(f);

            auto donor_counts = reader.getDonorCounts();
            auto donor_ids = reader.getDonorIds();
            auto donor_weights = reader.getDonorWeights();

            DcifChecker::throwIfCantRecoverLinearFunction(
                points, receptor_ids, donor_counts, donor_ids, donor_weights);
        }
    }

  private:

};

CREATE_INF_SUBCOMMAND(CheckDcifCommand)
