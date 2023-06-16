#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/MeshInterface.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/CommonSubcommandFunctions.h>
#include <parfait/GraphPlotter.h>

using namespace inf;

class StencilCommand : public SubCommand {
  public:
    std::string description() const override { return "write cell ids surrounding requested node"; }

    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), false, "");
        m.addParameter(Alias::preprocessor(), Help::preprocessor(), false, "NC_PreProcessor");
        m.addParameter(Alias::plugindir(), Help::plugindir(), false, getPluginDir());
        m.addParameter({"--golden", "-g"}, "The stencil file assumed to be correct", false, "");
        m.addParameter({"--check", "-c"}, "The stencil file to check against golden", false, "");
        m.addFlag({"--matrix"}, "Assume stencils are matrices and check those entries");
        m.addParameter({"--n2c"}, "write level 1 cells surrounding node", false, "");

        return m;
    }
    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        std::string bang = u8"\u2757 ERROR:";
        //        std::string check = u8"\u2714";
        std::string check = u8"\u2713";
        if (m.hasValid("golden")) {
            long num_wrong = 0;
            if (m.hasValid("check")) {
                if (m.has("matrix")) {
                    auto golden_graph = Parfait::Graph::readMatrix(m.get("golden"));
                    auto check_graph = Parfait::Graph::readMatrix(m.get("check"));
                    printf("Checking %ld rows from %s\n", check_graph.size(), m.get("check").c_str());
                    for (auto& pair : check_graph) {
                        auto row = pair.first;
                        auto& stencil = pair.second;
                        if (golden_graph.count(row) == 0) {
                            printf("%sGolden graph does not have row %ld in check file\n", bang.c_str(), row);
                            num_wrong++;
                        } else if (golden_graph.at(row) != stencil) {
                            printf("%s row %ld doesn't match golden.\n", bang.c_str(), row);
                            num_wrong++;
                        }
                    }
                    mp_rootprint("Number of incorrect rows: %ld\n", num_wrong);
                } else {
                    auto golden_graph = Parfait::Graph::read(m.get("golden"));
                    std::map<long, std::set<long>> check_graph = Parfait::Graph::read(m.get("check"));
                    printf("Checking %ld rows from %s\n", check_graph.size(), m.get("check").c_str());
                    for (auto& pair : check_graph) {
                        auto row = pair.first;
                        auto& stencil = pair.second;
                        if (golden_graph.count(row) == 0) {
                            printf("%sGolden graph does not have row %ld in check file\n", bang.c_str(), row);
                            num_wrong++;
                        } else if (golden_graph.at(row) != stencil) {
                            printf("%s row %ld doesn't match golden.\nGolden: <%s>\nvs\n Check: <%s>\n",
                                   bang.c_str(),
                                   row,
                                   Parfait::to_string(golden_graph.at(row)).c_str(),
                                   Parfait::to_string(stencil).c_str());
                            num_wrong++;
                        }
                    }
                    mp_rootprint("Number of incorrect rows: %ld\n", num_wrong);
                }
            }
        }
        if (m.hasValid("mesh")) {
            auto mesh = importMesh(m, mp);
            long node_id = m.getInt("n2c");
            auto n2c = inf::NodeToCell::build(*mesh);
            printf("\n");
            for (auto cell : n2c[node_id]) {
                printf("%d ", cell);
            }
        }
    }
};

CREATE_INF_SUBCOMMAND(StencilCommand)
