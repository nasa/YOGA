#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>
#include <t-infinity/PluginLocator.h>
#include <t-infinity/MeshLoader.h>
#include <parfait/LinearPartitioner.h>
#include <parfait/ExtentBuilder.h>
#include "../shared/ReaderFactory.h"
#include <t-infinity/Snap.h>
namespace inf {

class ExamineCommand : public SubCommand {
  public:
    std::string description() const override { return "get grid info"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addParameter(Alias::mesh(), Help::mesh(), true);
        m.addFlag({"t", "tags"}, "list surface tags");
        m.addFlag({"e", "extent"}, "print xyz dimensions of grid");
        m.addParameter({"s", "snap"}, "print fields in snap file", false);
        m.addParameter(edgeLengths(), "edge statistics for boundary tags", false);
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        if (mp.Rank() == 0) {
            auto grid_filename = m.get(Alias::mesh().front());
            auto reader = ReaderFactory::getReaderByFilename(grid_filename);

            long node_count = reader->nodeCount();

            std::map<inf::MeshInterface::CellType, long> cell_counts;
            for (auto& type : reader->cellTypes()) cell_counts[type] = reader->cellCount(type);

            long total_cells = 0;
            for (auto& pair : cell_counts) total_cells += pair.second;

            printf("Nodes in grid      : %s\n", Parfait::bigNumberToStringWithCommas(node_count).c_str());
            printf("Total cells in grid: %s\n", Parfait::bigNumberToStringWithCommas(total_cells).c_str());
            for (auto& pair : cell_counts) {
                printf("%s: %s\n",
                       MeshInterface::cellTypeString(pair.first).c_str(),
                       Parfait::bigNumberToStringWithCommas(pair.second).c_str());
            }

            if (m.has("tags")) {
                std::set<int> tags;

                long chunk_size = 10000;
                for (auto pair : cell_counts) {
                    auto type = pair.first;
                    long ncells = pair.second;
                    if (MeshInterface::is2DCellType(type)) {
                        int num_chunks = ncells / chunk_size + 1;
                        for (int chunk = 0; chunk < num_chunks; chunk++) {
                            auto range = Parfait::LinearPartitioner::getRangeForWorker(chunk, ncells, num_chunks);
                            auto v = reader->readCellTags(pair.first, range.start, range.end);
                            for (long tag : v) tags.insert(tag);
                        }
                    }
                }

                printf("Cell tags:\n");
                std::string formatted_tags;
                for (auto tag : tags) formatted_tags.append(std::to_string(tag) + " ");
                auto lines = Parfait::StringTools::wrapLines(formatted_tags, 40);
                for (auto& line : lines) mp_rootprint("%s\n", line.c_str());
            }

            if (m.has("extent")) {
                printf("Mesh bounds:\n");
                auto extent = Parfait::ExtentBuilder::createEmptyBuildableExtent<double>();
                long chunk_size = 10000;
                int num_chunks = node_count / chunk_size + 1;
                for (int chunk = 0; chunk < num_chunks; chunk++) {
                    auto range = Parfait::LinearPartitioner::getRangeForWorker(chunk, node_count, num_chunks);
                    auto v = reader->readCoords(range.start, range.end);
                    for (auto& p : v) Parfait::ExtentBuilder::add(extent, p);
                }
                printf("%s\n", extent.to_string().c_str());
            }

            if (m.has("snap")) {
                auto fields = loadSnapFields(mp.getCommunicator(), m.get("snap"), *reader);
                printf("snap fields:\n");
                for (const auto& f : fields) {
                    printf("  %s\n", f.c_str());
                }
            }

            if (m.has(edgeLengths())) {
                auto tags = Parfait::StringTools::parseIntegerList(m.get(edgeLengths()));
                auto pp = inf::getMeshLoader(inf::getPluginDir(), "NC_PreProcessor");
                auto mesh = pp->load(mp.getCommunicator(), grid_filename);
                calcAndPrintFaceStats(*mesh, tags);
            }
        }
    }

    std::vector<std::string> loadSnapFields(MPI_Comm comm, const std::string& filename, const Reader& reader) const {
        Snap snap(comm);

        {
            auto cell_ids = getGlobalCellIds(reader);
            std::vector<bool> do_own(cell_ids.size(), true);
            snap.setTopology(FieldAttributes::Cell(), cell_ids, do_own);
        }
        {
            auto node_ids = getGlobalNodeIds(reader);
            std::vector<bool> do_own(node_ids.size(), true);
            snap.setTopology(FieldAttributes::Node(), node_ids, do_own);
        }

        snap.load(filename);
        return snap.availableFields();
    }

    std::vector<long> getGlobalCellIds(const Reader& reader) const {
        std::set<int> cell_ids;
        for (auto cell_type : reader.cellTypes()) {
            auto ids = reader.readCells(cell_type, 0, reader.cellCount(cell_type));
            cell_ids.insert(ids.begin(), ids.end());
        }
        return {cell_ids.begin(), cell_ids.end()};
    }
    std::vector<long> getGlobalNodeIds(const Reader& reader) const {
        std::vector<long> node_ids(reader.nodeCount());
        for (long node_id = 0; node_id < reader.nodeCount(); ++node_id) {
            node_ids[node_id] = node_id;
        }
        return node_ids;
    }

  private:
    void calcAndPrintFaceStats(const inf::MeshInterface& mesh, const std::vector<int>& tags_to_check) const {
        std::set<int> tags(tags_to_check.begin(), tags_to_check.end());
        double shortest_edge = 5e30, longest_edge = 0, avg_edge = 0;
        long edge_count = 0;
        std::vector<int> face;
        for (int cell_id = 0; cell_id < mesh.cellCount(); cell_id++) {
            if (tags.count(mesh.cellTag(cell_id)) == 1) {
                auto cell_type = mesh.cellType(cell_id);
                if (inf::MeshInterface::TRI_3 == cell_type or inf::MeshInterface::QUAD_4 == cell_type) {
                    mesh.cell(cell_id, face);
                    for (size_t i = 0; i < face.size(); i++) {
                        edge_count++;
                        Parfait::Point<double> a, b;
                        int left = face[i];
                        int right = face[(i + 1) % face.size()];
                        mesh.nodeCoordinate(left, a.data());
                        mesh.nodeCoordinate(right, b.data());
                        double length = (a - b).magnitude();
                        shortest_edge = std::min(shortest_edge, length);
                        longest_edge = std::max(longest_edge, length);
                        avg_edge += length;
                    }
                }
            }
        }
        avg_edge /= double(edge_count);
        printf("Edge lengths for tags: ");
        for (int tag : tags) printf("%i ", tag);
        printf("\n");
        printf("min: %e\n", shortest_edge);
        printf("max: %e\n", longest_edge);
        printf("avg: %e\n", avg_edge);
    }

    static std::vector<std::string> edgeLengths() { return {"edge-stats"}; }
};
}

CREATE_INF_SUBCOMMAND(inf::ExamineCommand)