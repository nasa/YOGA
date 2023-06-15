#include <parfait/Point.h>
#include <parfait/TecplotWriter.h>
#include <parfait/TecplotBinaryReader.h>
#include <parfait/TecplotBinaryWriter.h>
#include <RingAssertions.h>

TEST_CASE("Tecplot writer tests") {
    long num_points = 4;
    long num_cells = 1;
    std::vector<Parfait::Point<double>> points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    auto getPoint = [&](long id, double* p) {
        p[0] = points[id][0];
        p[1] = points[id][1];
        p[2] = points[id][2];
    };

    auto getCellNodes = [](int cell_id, int* cell_nodes) {
        cell_nodes[0] = 0;
        cell_nodes[1] = 1;
        cell_nodes[2] = 2;
        cell_nodes[3] = 3;
    };

    std::vector<double> node_field = {0, 1, 2, 3};
    auto getNodeField = [&](long node_id) { return node_field[node_id]; };

    // To prevent race condition when tests are run in parallel
    auto filename = Parfait::StringTools::randomLetters(5) + "-";

    SECTION("single tet") {
        auto getCellType = [](long cell_id) { return Parfait::Visualization::CellType::TET; };

        filename += "single-tet-test";

        Parfait::tecplot::Writer writer(filename, num_points, num_cells, getPoint, getCellType, getCellNodes);
        writer.addNodeField("dog", getNodeField);
        writer.addNodeField("dog2", getNodeField);
        writer.visualize();
        REQUIRE(0 == remove((filename + ".tec").c_str()));
    }
    SECTION("single quad") {
        auto getCellType = [](long cell_id) { return Parfait::Visualization::CellType::QUAD; };

        filename += "single-quad-test";

        Parfait::tecplot::Writer writer(filename, num_points, num_cells, getPoint, getCellType, getCellNodes);
        writer.addNodeField("cat", getNodeField);
        writer.addNodeField("cat2", getNodeField);
        writer.visualize();
        REQUIRE(0 == remove((filename + ".tec").c_str()));
    }
    SECTION("write and read single tet binary") {
        auto getCellType = [](long cell_id) { return Parfait::Visualization::CellType::TET; };

        filename += "single-tet-test-binary.plt";

        Parfait::tecplot::BinaryWriter writer(filename, num_points, num_cells, getPoint, getCellType, getCellNodes);
        writer.addNodeField("dog", getNodeField);
        writer.addNodeField("dog2", getNodeField);
        writer.visualize();

        Parfait::tecplot::BinaryReader reader(filename);
        reader.read();
        auto min_max = reader.getRangeOfField("X");
        REQUIRE(min_max[0] == 0.0);
        REQUIRE(min_max[1] == 1.0);
        auto points_read = reader.getPoints();
        REQUIRE(points_read.size() == points.size());
        for (int n = 0; n < int(points.size()); n++) {
            for (int i = 0; i < 3; i++) {
                REQUIRE(points[n][i] == points_read[n][i]);
            }
        }

        auto dog = reader.getField("dog");
        REQUIRE(dog.size() == points.size());
        for (int n = 0; n < int(points.size()); n++) {
            REQUIRE(dog[n] == getNodeField(n));
        }
        REQUIRE(0 == remove(filename.c_str()));
    }
}
