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

class MetricCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Metric operations"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(Alias::help(), Help::help());
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addFlag({"implied"}, "export implied metric for mesh");
        m.addFlag({"intersect"}, "intersect multiple metric fields from select");
        m.addFlag({"average"}, "combine multiple metric fields via log-euclidean average");
        m.addParameter(
            {"weights"}, "optional weights for each metric for log-euclidean averaging", false);
        m.addParameter({"select"}, "Fields to intersect", false);
        m.addParameter({"complexity"}, "target mesh complexity", false);
        m.addParameter({"aspect-ratio"}, "request a global aspect ratio limit", false, "-1.0");
        m.addParameter({"gradation"}, "request a global gradation", false, "-1.0");
        m.addFlag({"shock-filtering"}, "request shock filtering");
        m.addParameter({"target-node-count"}, "Compute target size by node count", false);
        m.addParameter({"target-cell-count"}, "Compute target size by cell count", false);
        m.addParameter({"normal-spacing"},
                       "Set normal spacing at viscous walls (use with --tags options)",
                       false);
        m.addParameter({"t", "tags"}, "tags in the mesh that correspond to viscous walls", false);
        m.addParameter({"metrics"}, "input metrics", false);
        m.addParameter(Alias::snap(), "input scalar fields as snap files", false);
        m.addParameter(Alias::outputFileBase(), "output filename", false, "metric.snap");
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        return m;
    }

    double determineTargetComplexity(Parfait::CommandLineMenu& m,
                                     const MeshInterface& mesh,
                                     MessagePasser mp) {
        if (m.has("complexity")) {
            return m.getDouble("complexity");
        }
        if (m.has("target-node-count")) {
            return m.getDouble("target-node-count") * 0.5;
        }
        if (m.has("target-cell-count")) {
            return m.getDouble("target-cell-count") * 3.0;
        }
        return calcDefaultComplexityFromGlobalNodeCount(mesh, mp);
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto meshfile = m.get(Alias::mesh());
        auto mesh_loader = selectDefaultPreProcessorFromMeshFilename(meshfile);
        auto outfile = m.get(Alias::outputFileBase());
        auto mesh = shortcut::loadMesh(mp, meshfile, mesh_loader);
        std::shared_ptr<FieldInterface> output_metric = nullptr;
        std::vector<std::shared_ptr<FieldInterface>> input_metrics;
        double target_complexity = determineTargetComplexity(m, *mesh, mp);
        if (m.has("implied")) {
            mp_rootprint("Calculating implied output_metric\n");
            output_metric = MetricManipulator::calcImpliedMetricAtNodes(
                *mesh, maxCellDimensionality(mp, *mesh));
            double implied_complexity =
                MetricManipulator::calcComplexity(mp, *mesh, *output_metric);
            mp_rootprint("Target complexity: %e  Implied metric complexity: %e\n",
                         target_complexity,
                         implied_complexity);
        }
        if (m.has("metrics")) {
            auto metric_filenames = m.getAsVector("metrics");
            for (auto& filename : metric_filenames) {
                input_metrics.push_back(loadMetricFromSnapFile(mp, *mesh, filename));
                double complexity =
                    MetricManipulator::calcComplexity(mp, *mesh, *input_metrics.back());
                mp_rootprint(
                    "Imported metric: %s with complexity: %e\n", filename.c_str(), complexity);
            }
        }
        if (m.has("snap")) {
            auto fields = inf::importFields(mp, mesh, m);
            auto metric_calculator = inf::getMetricCalculator(getPluginDir(), "RefinePlugins");
            Parfait::Dictionary settings;
            settings["complexity"] = target_complexity;
            settings["aspect ratio"] = m.getDouble("aspect-ratio");
            settings["gradation"] = m.getDouble("gradation");
            settings["shock filtering"] = m.has("shock-filtering");
            for (auto& field : fields) {
                // Note: field could be a scalar (1 element)
                // or could be a hessian (9 elements)
                // or could be a metric (6 elements).
                // RefinePlugins::metric_calculator will
                // switch what it does to calculate the metric
                // based on what you send it.
                auto metric = metric_calculator->calculate(
                    mesh, mp.getCommunicator(), field, settings.dump());
                input_metrics.push_back(metric);
            }
        }
        if (m.has("intersect")) {
            output_metric = intersectMetrics(mp, input_metrics);
        } else if (m.has("average")) {
            std::vector<double> weights(input_metrics.size(), 1.0);
            if (m.has("weights")) {
                auto w = m.getAsVector("weights");
                if (w.size() == weights.size())
                    for (int i = 0; i < int(w.size()); i++)
                        weights[i] = Parfait::StringTools::toDouble(w[i]);
                else
                    mp_rootprint(
                        "Expected %li weights, but %li provided\n", weights.size(), w.size());
            }

            output_metric = averageMetrics(mp, input_metrics, weights);
        }

        if (m.has("normal-spacing")) {
            output_metric = applyNormalSpacing(mp, m, mesh, target_complexity, *output_metric);
        }

        if (output_metric == nullptr and not input_metrics.empty())
            output_metric = input_metrics.front();

        if (output_metric != nullptr) {
            mp_rootprint("scaling to target complexity %e\n", target_complexity);
            output_metric = MetricManipulator::scaleToTargetComplexity(
                mp, *mesh, *output_metric, target_complexity);
            mp_rootprint("exporting to: %s\n", outfile.c_str());
            dumpByExtension(mp, mesh, outfile, output_metric);
        }
    }

    std::shared_ptr<FieldInterface> applyNormalSpacing(const MessagePasser& mp,
                                                       const Parfait::CommandLineMenu& m,
                                                       std::shared_ptr<MeshInterface> mesh,
                                                       double target_complexity,
                                                       const FieldInterface& metric) const {
        PARFAIT_ASSERT(
            m.has("tags"),
            "The normal-spacing option requires viscous wall tags set with --tags option.");
        auto target_spacing = m.getDouble("normal-spacing");
        auto wall_tags = Parfait::SetTools::toSet<int>(getTags(m));
        mp_rootprint("Setting target spacing: %e on tags:", target_spacing);
        for (int tag : wall_tags) mp_rootprint(" %d", tag);
        mp_rootprint("\n");

        auto metric_with_spacing = MetricManipulator::setPrescribedSpacingOnMetric(
            mp, *mesh, metric, wall_tags, target_complexity, target_spacing);
        auto metric_calculator = inf::getMetricCalculator(getPluginDir(), "RefinePlugins");

        mp_rootprint("Applying gradation to metric\n");
        Parfait::Dictionary settings;
        settings["complexity"] = target_complexity;
        return metric_calculator->calculate(
            mesh, mp.getCommunicator(), metric_with_spacing, settings.dump());
    }

    std::shared_ptr<FieldInterface> intersectMetrics(
        const MessagePasser& mp,
        std::vector<std::shared_ptr<FieldInterface>>& input_metrics) const {
        mp_rootprint("Intersecting:\n") for (auto field : input_metrics) {
            mp_rootprint("input field: %s\n", field->name().c_str())
        }
        auto output_metric = input_metrics.front();
        for (size_t i = 1; i < input_metrics.size(); i++) {
            auto metric_a = output_metric;
            auto metric_b = input_metrics[i];
            output_metric = MetricManipulator::intersect(*metric_a, *metric_b);
        }
        return output_metric;
    }

    std::shared_ptr<FieldInterface> averageMetrics(
        const MessagePasser& mp,
        std::vector<std::shared_ptr<FieldInterface>>& input_metrics,
        const std::vector<double>& weights) const {
        mp_rootprint("Averaging:\n") for (int i = 0; i < int(input_metrics.size()); i++) {
            mp_rootprint(
                "input field: %s (weight %f)\n", input_metrics[i]->name().c_str(), weights[i])
        }
        auto output_metric = MetricManipulator::logEuclideanAverage(input_metrics, weights);
        return output_metric;
    }

  private:
    double calcDefaultComplexityFromGlobalNodeCount(const MeshInterface& mesh, MessagePasser mp) {
        long owned_nodes = 0;
        for (int i = 0; i < mesh.nodeCount(); i++)
            if (mesh.ownedNode(i)) owned_nodes++;
        long total_nodes = mp.ParallelSum(owned_nodes);
        return 0.5 * double(total_nodes);
    }

    void dumpByExtension(MessagePasser mp,
                         std::shared_ptr<MeshInterface> mesh,
                         const std::string outfile,
                         std::shared_ptr<FieldInterface> metric) {
        auto extension = Parfait::StringTools::getExtension(outfile);
        if ("snap" == extension) {
            dumpMetricToSnap(mp, *mesh, outfile, metric);
        } else if ("pcd" == extension) {
            dumpAsPointwiseSources(mp, *mesh, outfile, metric);
        } else if ("solb" == extension) {
            shortcut::visualize(outfile, mp, mesh, {metric});
        } else {
            mp_rootprint("Unrecognized extension for metric: %s\n", extension.c_str());
        }
    }

    void dumpMetricToSnap(MessagePasser mp,
                          const MeshInterface& mesh,
                          const std::string filename,
                          std::shared_ptr<FieldInterface> metric) {
        Snap snap(mp.getCommunicator());
        snap.addMeshTopologies(mesh);
        snap.add(metric);
        snap.writeFile(filename);
    }

    std::vector<double> calcAvgEdgeLengths(const MeshInterface& mesh) {
        auto n2n = inf::NodeToNode::build(mesh);
        std::vector<double> avg_nbr_edges;
        for (int i = 0; i < mesh.nodeCount(); i++) {
            double avg = 0.0;
            Parfait::Point<double> a{}, b{};
            mesh.nodeCoordinate(i, a.data());
            auto& nbrs = n2n[i];
            for (int nbr : nbrs) {
                mesh.nodeCoordinate(nbr, b.data());
                avg += a.distance(b);
            }
            avg /= double(nbrs.size());
            avg_nbr_edges.push_back(avg);
        }
        return avg_nbr_edges;
    }

    void dumpAsPointwiseSources(MessagePasser mp,
                                const MeshInterface& mesh,
                                const std::string& filename,
                                std::shared_ptr<FieldInterface> metric) {
        std::vector<double> v(6, 0);
        auto points = inf::extractOwnedPoints(mesh);
        std::vector<double> isotropic_spacing;
        auto avg_edge_length = calcAvgEdgeLengths(mesh);
        for (int i = 0; i < mesh.nodeCount(); i++) {
            if (not mesh.ownedNode(i)) continue;
            metric->value(i, v.data());
            auto tensor = MetricManipulator::expandToMatrix(v.data());
            auto decomp = Parfait::MetricDecomposition::decompose(tensor);
            std::array<double, 3> h{};
            for (int j = 0; j < 3; j++) h[j] = 1.0 / std::sqrt(decomp.D(j, j));
            std::sort(h.begin(), h.end());
            double spacing = std::max(h[1], 0.2 * avg_edge_length[i]);
            isotropic_spacing.push_back(spacing);
        }

        auto all_points = Parfait::flatten(mp.Gather(points, 0));
        auto all_spacings = Parfait::flatten(mp.Gather(isotropic_spacing, 0));
        if (0 == mp.Rank()) {
            std::vector<double> decay(all_points.size(), 0.5);
            Parfait::PointwiseSources pointwise_sources(all_points, all_spacings, decay);
            pointwise_sources.write(filename);
        }
    }
};

CREATE_INF_SUBCOMMAND(MetricCommand)
