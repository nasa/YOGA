#include "CubeSampling.h"
#include <t-infinity/CommonSubcommandFunctions.h>

using namespace inf;
class CubeSamplingSubCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Samples a CFD grid and solution onto a cartesian structured grid."; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "ParfaitViz");
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::snap(), Help::snap(), false);
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter({"o"}, "Output filename", false, "out");
        m.addParameter({"select"}, "Only include these fields in the output file.", false);
        m.addParameter({"at"}, "Convert fields to either <nodes> or <cells>", false);
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto donor_mesh = importMesh(m, mp);
        auto fields = importFields(mp, donor_mesh, m);

        auto cube = inf::CartMesh::create(mp, 15, 15, 15, {{0, 0, 0}, {10, 10, 10}});

        std::vector<Parfait::Point<double>> points(cube->nodeCount());
        for (int i = 0; i < cube->nodeCount(); i++) {
            points[i] = cube->node(i);
        }

        std::vector<YOGA::DonorCloud> donor_stencils = generateDonorStencilsForPoints(*donor_mesh.get(), points);

        std::vector<std::vector<double>> sampled_fields;
        std::vector<std::string> field_names;

        for (auto f : fields) {
            auto some_field = convertFieldToVector(*f);
            std::vector<double> sampled_field(points.size());
            for (size_t i = 0; i < points.size(); i++) {
                sampled_field[i] = 0.0;
                for (size_t j = 0; j < donor_stencils[i].node_ids.size(); j++) {
                    int neighbor = donor_stencils[i].node_ids[j];
                    double w = donor_stencils[i].weights[j];
                    sampled_field[i] += w * some_field[neighbor];
                }
            }
            sampled_fields.push_back(sampled_field);
            field_names.push_back(f->name());
        }

        inf::shortcut::visualize_at_nodes("out", mp, cube, sampled_fields, field_names);
    }
};

CREATE_INF_SUBCOMMAND(CubeSamplingSubCommand)