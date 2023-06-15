#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/Extract.h>
#include <t-infinity/Shortcuts.h>
#include <parfait/STL.h>
#include <parfait/SetTools.h>
#include <parfait/VectorTools.h>
#include <t-infinity/BetterDistanceCalculator.h>
#include <t-infinity/DistanceCalculator.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <parfait/Timing.h>

namespace inf  {
class DistanceCommand : public SubCommand {
  public:
    std::string description() const override { return "generate distance to the wall"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag({"--version"}, "print version and exit");
        m.addParameter({"--tags", "-t"}, "1,2,3 = get distance to tags 1 2 and 3", true);
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "default");
        m.addParameter(Alias::postprocessor(), Help::postprocessor(), false, "default");
        m.addParameter(Alias::outputFileBase(), Help::outputFileBase(), false, "distance");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter({"--at"}, {"Distance at nodes or cells"}, false, "nodes");

        m.addHiddenParameter({"--viz"}, "Write a vizualization file of distance and search cost measurements");
        m.addHiddenParameter({"--max-depth"}, "max levels in oct-tree", "10");
        m.addHiddenParameter({"--max-objects-per-voxel"}, "", "10");
        m.addHiddenParameter({"--num-trees"}, "Number of division trees to use", "5");
        m.addHiddenFlag({"--stl"}, "Write STL of surface");
        m.addHiddenFlag({"--no-export"}, "skip writing any output files");
        m.addHiddenFlag({"--compare-old"}, "Run the old algorithm for comparison");
        return m;
    }

    std::string getMeshBaseName(Parfait::CommandLineMenu m) const {
        std::string mesh_filename = m.get("mesh");
        std::string output_name = Parfait::StringTools::getPath(mesh_filename) +
                                  Parfait::StringTools::stripExtension(Parfait::StringTools::stripPath(mesh_filename),
                                                                       {"meshb", "lb8.ugrid", "b8.ugrid", "ugrid"});
        return output_name;
    }

    template <typename Container>
    void writeSTL(MessagePasser mp, const Container& facets) const {
        Container all_facets;
        mp.Gather(facets, all_facets);
        if (mp.Rank() == 0) {
            Parfait::STL::write(all_facets, "surface");
        }
    }

    std::string whereToSearch(MessagePasser mp, Parfait::CommandLineMenu m) const {
        auto at = m.get("at");
        at = Parfait::StringTools::tolower(at);
        if (at == "nodes" or at == "node") return inf::FieldAttributes::Node();
        if (at == "cells" or at == "cell") return inf::FieldAttributes::Cell();
        if (mp.Rank() == 0) {
            PARFAIT_WARNING("Unknown option for where to get search distance " + at);
            PARFAIT_WARNING("Valid options are <nodes> or <cells>");
            PARFAIT_WARNING("Progressing assuming you meant nodes");
        }
        return inf::FieldAttributes::Node();
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        if (m.has("--version")) {
            exit(0);
        }

        auto begin = Parfait::Now();
        auto mesh = importMesh(m, mp);
        auto end = Parfait::Now();
        mp_rootprint("Importing mesh took %s\n\n", Parfait::readableElapsedTimeAsString(begin, end).c_str());

        std::set<int> tags = Parfait::SetTools::toSet<int>(getTags(m));
        auto all_tags = inf::extractAllTagsWithDimensionality(mp, *mesh, 2);
        mp_rootprint("Mesh has tags %s\nSearching tags %s\n",
                     Parfait::to_string(all_tags).c_str(),
                     Parfait::to_string(tags).c_str());

        if (m.has("stl")) {
            auto facets = inf::extractOwned2DFacets(mp, *mesh, tags);
            writeSTL(mp, facets);
        }

        int num_trees = m.getInt("num-trees");
        int max_depth = m.getInt("max-depth");
        int max_objects_per_voxel = m.getInt("max-objects-per-voxel");

        std::vector<double> distance, search_cost, nearest_surface_tag;
        std::vector<int> nearest_tag_as_int;

        std::vector<Parfait::Point<double>> query_points;
        std::string search_at = whereToSearch(mp, m);
        if (search_at == inf::FieldAttributes::Node())
            query_points = inf::extractPoints(*mesh);
        else if (search_at == inf::FieldAttributes::Cell())
            query_points = inf::extractCellCentroids(*mesh);

        std::tie(distance, search_cost, nearest_tag_as_int) =
            inf::calcBetterDistance(mp, *mesh, tags, query_points, num_trees, max_depth, max_objects_per_voxel);

        nearest_surface_tag = Parfait::VectorTools::to_double(nearest_tag_as_int);

        if (m.has("compare-old")) {
            inf::calcDistance(mp, *mesh, tags, query_points, max_depth, max_objects_per_voxel);
        }

        if (m.has("no-export")) {
            mp.Finalize();
            exit(0);
        }

        auto distance_as_field = std::make_shared<inf::VectorFieldAdapter>("distance", search_at, distance);
        exportMeshAndFields(m.get("o"), mesh, m, mp, {distance_as_field});

        if (m.has("viz")) {
            inf::shortcut::visualize(m.get("viz"),
                                          mp,
                                          mesh,
                                          search_at,
                                          {distance, search_cost, nearest_surface_tag},
                                          {"distance", "search-cost", "nearest-surface-tag"});
        }
    }
};
}

CREATE_INF_SUBCOMMAND(inf::DistanceCommand)