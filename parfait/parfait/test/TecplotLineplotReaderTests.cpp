#include <RingAssertions.h>
#include <parfait/FileTools.h>
#include <parfait/DataFrame.h>
#include <parfait/DataFrameTools.h>
#include <parfait/PointWriter.h>
#include <parfait/LinePlotters.h>

TEST_CASE("Read tecplot 3d point cloud") {
    std::string filename = std::string(GRIDS_DIR) + "/profile/duct_3d_sa.prf";
    Parfait::TecplotDataFrameReader reader(filename);

    auto column_names = reader.column_names;
    REQUIRE(column_names[0] == "X");
    REQUIRE(column_names[1] == "Y");
    REQUIRE(column_names[2] == "Z");
    for (int i = 1; i <= 9; i++) {
        std::string expected_name = "V" + std::to_string(i);
        REQUIRE(column_names[i - 1 + 3] == expected_name);
    }

    auto df = reader.exportToFrame();
    REQUIRE(df.columns() == reader.column_names);
    REQUIRE(df["X"].size() == 832);
}

TEST_CASE("Read tecplot 3d point cloud, variables on separate lines") {
    std::string filename = std::string(GRIDS_DIR) + "/profile/duct_3d_sa_multiline.prf";
    Parfait::TecplotDataFrameReader reader(filename);

    auto column_names = reader.column_names;
    REQUIRE(column_names[0] == "X");
    REQUIRE(column_names[1] == "Y");
    REQUIRE(column_names[2] == "Z");
    for (int i = 1; i <= 9; i++) {
        std::string expected_name = "V" + std::to_string(i);
        REQUIRE(column_names[i - 1 + 3] == expected_name);
    }

    auto df = reader.exportToFrame();
    REQUIRE(df.columns() == reader.column_names);
    REQUIRE(df["X"].size() == 832);
}

TEST_CASE("plot dataframe if it is a point cloud") {
    std::string filename = std::string(GRIDS_DIR) + "/profile/duct_3d_sa.prf";
    auto df = Parfait::readTecplot(filename);

    auto points = Parfait::DataFrameTools::extractPoints(df);
    Parfait::PointWriter::write("duct_3d_sa.3D", points, df["V1"]);
}
