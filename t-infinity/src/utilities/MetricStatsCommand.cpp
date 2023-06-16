#include <stdio.h>
#include <t-infinity/SubCommand.h>
#include <parfait/Dictionary.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/MetricManipulator.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/Snap.h>
#include <t-infinity/Gradients.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <t-infinity/MetricCalculatorInterface.h>
#include <parfait/PointwiseSources.h>
#include <t-infinity/Extract.h>
#include <parfait/Flatten.h>
#include <t-infinity/MetricHelpers.h>
#include <t-infinity/MeshHelpers.h>

using namespace inf;

class MetricStatsCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Metric operations"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(), Help::help());
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter({"metric"}, "input metric snap file", true);
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto meshfile = m.get(Alias::mesh());
        auto mesh_loader = selectDefaultPreProcessorFromMeshFilename(meshfile);
        auto mesh = shortcut::loadMesh(mp, meshfile, mesh_loader);

        auto metric_filename = m.get("metric");
        auto metric_field = loadMetricFromSnapFile(mp, *mesh, metric_filename);
        auto metric = MetricManipulator::fromFieldToTensors(*metric_field);
        if(mp.Rank() == 0)
            printf("\n\nMetric Statistics:\n\n");
        calcAndPrintComplexity(mp, *mesh, *metric_field);
        calcAndPrintStretchingRatioStats(mp, *mesh, metric);
    }



  private:
    void calcAndPrintComplexity(MessagePasser mp, const MeshInterface& mesh,const FieldInterface& metric_field) {
        double complexity = MetricManipulator::calcComplexity(mp,mesh,metric_field);
        if(mp.Rank() == 0)
            printf("Complexity: %e  (%s)\n\n",complexity,Parfait::bigNumberToStringWithCommas(complexity).c_str());
    }

    void calcAndPrintStretchingRatioStats(MessagePasser mp, const MeshInterface& mesh,const std::vector<Tensor>& metric) {
        double min_sr = std::numeric_limits<double>::max();
        double max_sr = 0.0;
        double avg_sr = 0.0;
        for(auto& m:metric){
            auto decomp = Parfait::MetricDecomposition::decompose(m);
            double smallest_eig = decomp.D(0,0);
            double largest_eig = smallest_eig;
            for(int i=0;i<3;i++){
                double eig = decomp.D(i,i);
                smallest_eig = std::min(eig,smallest_eig);
                largest_eig = std::max(eig,largest_eig);
            }
            double shortest_radius = 1.0 / std::sqrt(largest_eig);
            double longest_radius = 1.0 / std::sqrt(smallest_eig);
            double stretching_ratio = longest_radius / shortest_radius;
            avg_sr += stretching_ratio;
            max_sr = std::max(stretching_ratio, max_sr);
            min_sr = std::min(stretching_ratio, min_sr);
        }
        avg_sr /= double(metric.size());
        max_sr = mp.ParallelMax(max_sr,0);
        min_sr = mp.ParallelMin(min_sr, 0);
        avg_sr = mp.ParallelAverage(avg_sr);
        if(mp.Rank() == 0) {
            printf("Stretching ratio:\n");
            printf("  min: %e\n", min_sr);
            printf("  max: %e\n", max_sr);
            printf("  avg: %e\n\n", avg_sr);
        }
    }
};

CREATE_INF_SUBCOMMAND(MetricStatsCommand)
