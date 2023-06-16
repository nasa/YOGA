#include <parfait/SetTools.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/AddMissingFaces.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/QuiltTags.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/MeshOptimization.h>
#include <t-infinity/MeshQualityMetrics.h>
#include <t-infinity/FieldStatistics.h>

using namespace inf;

class OptimizeCommand : public SubCommand {
  public:
    std::string description() const override { return "Try move nodes to optimize mesh quality."; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "optimized");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter({"--slip-tags", "-s"},
                       "Nodes on these tags will remain on the tagged surface",
                       false,
                       "");
        m.addParameter({"--iterations", "-i"}, "Number of iterations", false, "20");
        m.addFlag({"--smooth-only"}, "Ignore cost function while stepping");
        m.addParameter({"--threshold", "-t"},
                       "Hilbert cost threshold (cells with cost under threshold are not modified)",
                       false,
                       "0.5");
        m.addParameter({"--algorithm", "-a"}, "hilbert, or smooth", false, "hilbert");
        m.addParameter({"--omega", "-w"}, "omega damping for smooth algorithm", false, "0.1");
        m.addParameter({"--quilt-tags", "-q"}, "Treat quilted tags as one tag", false, "");
        m.addParameter(
            {"--beta", "-b"}, "Step size as fraction of average node radius", false, "0.01");
        m.addFlag({"--plot", "-p"}, "Plot after each iteration");
        m.addFlag({"--disable-warp", "-w"}, "Don't try to flatten warped quad faces.");
        return m;
    }
    std::shared_ptr<TinfMesh> importMesh(const Parfait::CommandLineMenu& m,
                                         const MessagePasser& mp) const {
        auto dir = m.get(Alias::plugindir());
        auto mesh_loader_plugin_name = m.get(Alias::preprocessor());
        auto mesh_filename = m.get(Alias::mesh());
        mp_rootprint("Importing mesh: %s\n", mesh_filename.c_str());
        mp_rootprint("Using plugin: %s\n", mesh_loader_plugin_name.c_str());
        auto pp = getMeshLoader(dir, mesh_loader_plugin_name);
        auto imported_mesh = pp->load(mp.getCommunicator(), m.get(Alias::mesh()));
        return std::make_shared<TinfMesh>(imported_mesh);
    }

    void plot(MessagePasser mp,
              std::string filename,
              std::shared_ptr<TinfMesh> mesh,
              const std::vector<double>& cell_cost,
              const std::vector<double>& node_cost,
              const std::vector<double>& cell_flattness_cost,
              const std::vector<double>& node_types) const {
        std::vector<std::shared_ptr<inf::FieldInterface>> inf_fields;
        inf_fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "Hilbert cost", inf::FieldAttributes::Cell(), 1, cell_cost));

        inf_fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "Flattness cost", inf::FieldAttributes::Cell(), 1, cell_flattness_cost));

        inf_fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "Hilbert cost", inf::FieldAttributes::Node(), 1, node_cost));

        inf_fields.push_back(std::make_shared<inf::VectorFieldAdapter>(
            "type", inf::FieldAttributes::Node(), 1, node_types));
        return inf::shortcut::visualize_volume(filename, mp, mesh, inf_fields);
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        auto mesh = importMesh(m, mp);
        auto orig_tags = mesh->mesh.cell_tags;

        if (m.has("-q")) {
            auto quilt_tags = Parfait::StringTools::parseIntegerList(m.get("--quilt-tags"));
            inf::quiltTagsAndDontCompact(mp, *mesh, {quilt_tags});
        }

        inf::MeshOptimization optimizer(mp, mesh.get());
        if (m.has("-a")) {
            auto algorithm = m.get("-a");
            if (algorithm != "smooth" and algorithm != "hilbert") {
                mp_rootprint(
                    "Encountered unexpected optimization algorithm: %s.  Options are hilbert or "
                    "smooth\n",
                    algorithm.c_str());
            } else {
                optimizer.setAlgorithm(m.get("-a"));
            }
        }
        if (m.has("-t")) {
            optimizer.setThreshold(m.getDouble("-t"));
        }
        if (m.has("-w")) {
            optimizer.setOmega(m.getDouble("-w"));
        }
        if (m.has("-b")) {
            optimizer.setBeta(m.getDouble("-b"));
        }
        if (m.has("-w")) {
            optimizer.disableWarp();
        }
        if (m.has("--smooth-only")) {
            optimizer.smooth_only = true;
        }
        if (m.has("--slip-tags")) {
            auto slip_tags = Parfait::StringTools::parseIntegerList(m.get("--slip-tags"));
            optimizer.setSlipTags(Parfait::SetTools::toSet<int>(slip_tags));
        }

        optimizer.finalize();

        int iter = m.getInt("-i");

        auto node_types = optimizer.getNodeTypes();

        auto cell_hilbert_cost = inf::MeshQuality::cellHilbertCost(*mesh);
        auto cell_flattness_cost = inf::MeshQuality::cellFlattnessCost(*mesh);
        auto node_hilbert_cost = inf::MeshQuality::nodeHilbertCost(*mesh, optimizer.n2c_volume);
        auto node_statistics = inf::FieldStatistics::calcNodes(mp, *mesh, node_hilbert_cost);
        auto cell_statistics = inf::FieldStatistics::calcVolumeCells(mp, *mesh, cell_hilbert_cost);
        mp_rootprint("\n%d", 0);
        inf::FieldStatistics::print(mp, node_statistics, "Hilbert Node Cost");
        inf::FieldStatistics::print(mp, cell_statistics, "Hilbert Cell Cost");

        if (m.has("--plot")) {
            plot(mp,
                 "opt." + std::to_string(0),
                 mesh,
                 cell_hilbert_cost,
                 node_hilbert_cost,
                 cell_flattness_cost,
                 node_types);
        }
        for (int i = 0; i < iter; i++) {
            optimizer.step();
            mp_rootprint("\n%d", i + 1);

            if (m.has("--plot")) {
                cell_hilbert_cost = inf::MeshQuality::cellHilbertCost(*mesh);
                node_hilbert_cost = inf::MeshQuality::nodeHilbertCost(*mesh, optimizer.n2c_volume);
                cell_flattness_cost = inf::MeshQuality::cellFlattnessCost(*mesh);
                node_statistics = inf::FieldStatistics::calcNodes(mp, *mesh, node_hilbert_cost);
                cell_statistics =
                    inf::FieldStatistics::calcVolumeCells(mp, *mesh, cell_hilbert_cost);
                inf::FieldStatistics::print(mp, node_statistics, "Hilbert Node Cost");
                inf::FieldStatistics::print(mp, cell_statistics, "Hilbert Cell Cost");
                plot(mp,
                     "opt." + std::to_string(i + 1),
                     mesh,
                     cell_hilbert_cost,
                     node_hilbert_cost,
                     cell_flattness_cost,
                     node_types);
            }
        }

        std::string output_meshname = m.get(Alias::outputFileBase());
        mesh->mesh.cell_tags = orig_tags;
        inf::ParallelUgridExporter::write(output_meshname, *mesh, mp);
    }
};

CREATE_INF_SUBCOMMAND(OptimizeCommand)
