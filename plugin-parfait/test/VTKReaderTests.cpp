#include <RingAssertions.h>
#include <parfait/MapTools.h>
#include <parfait/StringTools.h>
#include <t-infinity/CartMesh.h>
#include <t-infinity/Shortcuts.h>
#include <t-infinity/InfinityToVtk.h>
#include <shared/Reader.h>
#include "parfait/ByteSwap.h"

class VTKReader : public Reader {
  public:
    VTKReader(std::string filename) {
        f = fopen(filename.c_str(), "r");
        PARFAIT_ASSERT(f != nullptr, "could not open file for reading " + filename);
        buildTableOfContents();
    }
    ~VTKReader() {
        if (f) fclose(f);
    }

    long nodeCount() const override { return node_count; }
    long cellCount(inf::MeshInterface::CellType type) const override {
        if (cell_type_counts.count(type) != 0) return cell_type_counts.at(type);
        return 0;
    }
    virtual std::vector<Parfait::Point<double>> readCoords(long start, long end) const override {
        std::vector<Parfait::Point<double>> coords;
        return coords;
    }
    virtual std::vector<long> readCells(inf::MeshInterface::CellType type, long start, long end) const override {
        std::vector<long> cell_nodes;
        return cell_nodes;
    }
    virtual std::vector<int> readCellTags(inf::MeshInterface::CellType element_type,
                                          long element_start,
                                          long element_end) const override {
        std::vector<int> cell_tags;
        return cell_tags;
    }
    virtual std::vector<inf::MeshInterface::CellType> cellTypes() const override {
        auto keys = Parfait::MapTools::keys(cell_type_counts);
        return std::vector<inf::MeshInterface::CellType>{keys.begin(), keys.end()};
    }

  private:
    FILE* f = nullptr;
    long node_count = 0;
    std::map<inf::MeshInterface::CellType, long> cell_type_counts;
    std::map<std::string, uint64_t> table_of_contents;

    void pedanticallyCheckHeader() {
        std::string expected = "# vtk DataFile Version 3.0";
        std::string line;
        Parfait::FileTools::readNextLine(f, line);
        PARFAIT_ASSERT(expected == line, "Error!\nExpected: " + expected + "\nFound: " + line);

        expected = "Powered by T-infinity";
        Parfait::FileTools::readNextLine(f, line);
        PARFAIT_ASSERT(expected == line, "Error!\nExpected: " + expected + "\nFound: " + line);

        expected = "BINARY";
        Parfait::FileTools::readNextLine(f, line);
        PARFAIT_ASSERT(expected == line, "Error!\nExpected: " + expected + "\nFound: " + line);

        expected = "DATASET UNSTRUCTURED_GRID";
        Parfait::FileTools::readNextLine(f, line);
        PARFAIT_ASSERT(expected == line, "Error!\nExpected: " + expected + "\nFound: " + line);
    }

    void buildTableOfContents() {
        pedanticallyCheckHeader();
        auto line = Parfait::FileTools::readNextLine(f);
        sscanf(line.c_str(), "POINTS %ld double", &node_count);
        table_of_contents["points"] = ftell(f);
        long point_size = long(node_count) * 3 * sizeof(double);
        fseek(f, point_size, SEEK_CUR);

        line = Parfait::FileTools::readNextLine(f);  // one to chomp the leading newline on cell header
        line = Parfait::FileTools::readNextLine(f);
        long total_cell_count;
        size_t number_of_ints_in_buffer;
        sscanf(line.c_str(), "CELLS %li %zu", &total_cell_count, &number_of_ints_in_buffer);
        table_of_contents["cells"] = ftell(f);
        fseek(f, long(number_of_ints_in_buffer) * sizeof(int), SEEK_CUR);

        line = Parfait::FileTools::readNextLine(f);  // one to chomp the leading newline on cell header
        line = Parfait::FileTools::readNextLine(f);
        long num_cells_again;
        sscanf(line.c_str(), "CELL_TYPES %li", &num_cells_again);
        PARFAIT_ASSERT(num_cells_again == total_cell_count,
                       "Cell Count mismatch at CELL_TYPES. Likely this file format is not supported");
        table_of_contents["cell_types"] = ftell(f);
        for (long i = 0; i < total_cell_count; i++) {
            int type;
            fread(&type, sizeof(int), 1, f);
            bswap_32(&type);
            auto mesh_type = inf::vtkToInfinityCellType(type);
            cell_type_counts[mesh_type]++;
        }

        // Here's where we'd start reading field data.
        // line = Parfait::FileTools::readNextLine(f); // one to chomp the leading newline
        // for(int i = 0; i < 5; i++){
        //    line = Parfait::FileTools::readNextLine(f);
        //    printf("cell_data line[%d]: %s\n", i, line.c_str());
        //}
        // long cell_count_for_data;
        // sscanf(line.c_str(), "CELL_DATA %li", &cell_count_for_data);
        // PARFAIT_ASSERT(cell_count_for_data == total_cell_count, "Cell Count mismatch at CELL_DATA. Likely this file
        // format is not supported"); table_of_contents["cell_data_start"] = ftell(f);
    }
};

TEST_CASE("Can read a vtk mesh we wrote") {
    auto mp = MessagePasser(MPI_COMM_SELF);
    auto mesh = inf::CartMesh::create(mp, 10, 10, 10);
    std::string filename = "test-mesh-" + Parfait::StringTools::randomLetters(5) + ".vtk";
    inf::shortcut::visualize(filename, mp, mesh);
    Parfait::FileTools::waitForFile(filename);

    std::shared_ptr<VTKReader> reader = nullptr;

    if (mp.Rank() == 0) {
        reader = std::make_shared<VTKReader>(filename);
        printf("reader node count %ld\n", reader->nodeCount());
        printf("mesh node count %d\n", mesh->nodeCount());
        REQUIRE(reader->nodeCount() == mesh->nodeCount());
        REQUIRE(std::vector<inf::MeshInterface::CellType>{inf::MeshInterface::QUAD_4, inf::MeshInterface::HEXA_8} ==
                reader->cellTypes());
        REQUIRE(reader->cellCount(inf::MeshInterface::QUAD_4) == mesh->cellCount(inf::MeshInterface::QUAD_4));
        REQUIRE(reader->cellCount(inf::MeshInterface::HEXA_8) == mesh->cellCount(inf::MeshInterface::HEXA_8));
    }
}