#include <RingAssertions.h>
#include <parfait/DataFrame.h>
#include <parfait/LinePlotters.h>

using namespace Parfait;

TEST_CASE("Can build and access a Parfait::DataFrame") {
    DataFrame::MappedColumns column_data;
    column_data["pokeballs"] = {1.0, 2.0, 3.0};
    column_data["pokedexes"] = {0.1, 0.2, 0.3};

    DataFrame data_frame(column_data);

    std::vector<std::string> expected_columns = {"pokeballs", "pokedexes"};
    auto actual_columns = data_frame.columns();
    REQUIRE(expected_columns == actual_columns);

    REQUIRE(std::array<int, 2>{3, 2} == data_frame.shape());

    for (const auto& expected : column_data) {
        REQUIRE(expected.second == data_frame[expected.first]);
    }
}

TEST_CASE("Categorize based on a column") {
    DataFrame df;
    df["x"] = {.1, .4, .7};
    df["y"] = {.2, .5, .8};
    df["z"] = {.3, .6, .9};
    df["some-category"] = {0, 1, 2};

    auto split_dataframes = df.groupBy("some-category");

    REQUIRE(3 == split_dataframes.size());
    auto& df_0 = split_dataframes[0];
    auto& df_1 = split_dataframes[1];
    auto& df_2 = split_dataframes[2];
    REQUIRE(df.columns() == df_0.columns());
    REQUIRE(df.columns() == df_1.columns());
    REQUIRE(df.columns() == df_2.columns());

    REQUIRE(std::vector<double>{.1} == df_0["x"]);
    REQUIRE(std::vector<double>{.2} == df_0["y"]);
    REQUIRE(std::vector<double>{.3} == df_0["z"]);

    REQUIRE(std::vector<double>{.4} == df_1["x"]);
    REQUIRE(std::vector<double>{.5} == df_1["y"]);
    REQUIRE(std::vector<double>{.6} == df_1["z"]);

    REQUIRE(std::vector<double>{.7} == df_2["x"]);
    REQUIRE(std::vector<double>{.8} == df_2["y"]);
    REQUIRE(std::vector<double>{.9} == df_2["z"]);
}

TEST_CASE("Can select parts of dataframe by filtering") {
    DataFrame df;
    df["x"] = {0, 1, 2, 3, 4, 5};
    df["y"] = {1, 1, 1, 2, 2, 2};
    auto slice = df.select("x", 0, 3);
    REQUIRE(df.columns() == slice.columns());
    REQUIRE(std::vector<double>{0, 1, 2} == slice["x"]);
    REQUIRE(std::vector<double>{1, 1, 1} == slice["y"]);
}

TEST_CASE("Can parse a csv file with trailing commas") {
    std::string file_contents = "a, b,c, d ,\n";
    file_contents += "0, 1, 2, 3,\n";
    file_contents += "10, 11, 12, 13,\n";
    auto data_frame = LinePlotters::CSVReader::toDataFrame(file_contents);
    REQUIRE(data_frame.columns() == std::vector<std::string>{"a", "b", "c", "d"});
}

TEST_CASE("Data frame to string integration") {
    DataFrame df;
    df["x"] = {1};
    df["y"] = {2};
    auto s = LinePlotters::CSVWriter::dataFrameToString(df);
    REQUIRE("x,y\n1,2" == s);
}

TEST_CASE("Can read and write data frame to csv in parallel") {
    DataFrame df;
    MessagePasser mp(MPI_COMM_WORLD);
    std::string filename = std::to_string(mp.NumberOfProcesses()) + "_parallel.csv";

    double expected_x = {double(mp.Rank())};
    double expected_y = 2 * expected_x;
    df["x"] = {expected_x, expected_x};
    df["y"] = {expected_y, expected_y};
    LinePlotters::CSVWriter::write(mp, filename, df);
    Parfait::FileTools::waitForFile(filename, 5);
    auto read_df = LinePlotters::CSVReader::read(mp, filename);

    REQUIRE(read_df.columns().size() == 2);

    double x = read_df["x"][0];
    double y = read_df["y"][0];
    REQUIRE(expected_x == x);
    REQUIRE(expected_y == y);

    if (mp.Rank() == 0) REQUIRE(0 == remove(filename.c_str()));
}

TEST_CASE("Can read in a dataframe with less than nproc things in it") {
    MessagePasser mp(MPI_COMM_WORLD);
    std::string filename = std::to_string(mp.NumberOfProcesses()) + "_small_data_frame.csv";
    if (mp.Rank() == 0) {
        DataFrame df;
        df["x"] = {0.0};
        LinePlotters::CSVWriter::write(filename, df);
        FileTools::waitForFile(filename, 5);
    }
    mp.Barrier();

    auto parallel_df = LinePlotters::CSVReader::read(mp, filename);
    if (mp.Rank() == 0) {
        REQUIRE(1 == parallel_df.shape()[0]);
    } else {
        REQUIRE(0 == parallel_df.shape()[0]);
    }

    if (mp.Rank() == 0) REQUIRE(0 == remove(filename.c_str()));
}

TEST_CASE("CSV Reading will warn if data is different length than headers") {
    std::string file_contents = "a,b,c\n";
    file_contents += "0, 1, 2,\n";
    file_contents += "10, 11, 12, 13\n";  // This line is too long
    REQUIRE_THROWS_WITH(LinePlotters::CSVReader::toDataFrame(file_contents), Contains("too many columns"));
}

TEST_CASE("Can write files from a Parfait::DataFrame") {
    MessagePasser mp(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;

    DataFrame::MappedColumns column_data;
    column_data["Digimon cards"] = {1.0, 2.0, 3.0};
    column_data["Pokemon cards"] = {0.1, 0.2, 0.3};

    DataFrame data_frame(column_data);
    SECTION("write CSV file") {
        writeCSV("test.csv", data_frame);
        DataFrame actual = readCSV("test.csv");
        REQUIRE(std::array<int, 2>{3, 2} == actual.shape());
        for (const auto& column : column_data) {
            REQUIRE(column.second == actual[column.first]);
        }

        REQUIRE(0 == remove("test.csv"));
    }
    SECTION("append CSV file") {
        writeCSV("appended.csv", data_frame);
        appendCSV("appended.csv", data_frame);
        auto actual = readCSV("appended.csv");
        REQUIRE(std::array<int, 2>{6, 2} == actual.shape());
        for (const auto& column : column_data) {
            auto expected = column.second;
            expected.insert(expected.end(), column.second.begin(), column.second.end());
            REQUIRE(expected == actual[column.first]);
        }
        REQUIRE(0 == remove("appended.csv"));
    }
    SECTION("write Tecplot file") {
        writeTecplot("test.dat", data_frame);
        std::vector<std::string> expected = {R"(TITLE="Powered By Parfait")",
                                             R"(VARIABLES="Digimon cards", "Pokemon cards")",
                                             R"(ZONE T="test")",
                                             R"(1.000000 0.100000)",
                                             R"(2.000000 0.200000)",
                                             R"(3.000000 0.300000)"};
        auto actual = StringTools::split(FileTools::loadFileToString("test.dat"), "\n");
        REQUIRE(expected == actual);
        REQUIRE(0 == remove("test.dat"));
    }
    SECTION("append Tecplot file") {
        writeTecplot("append.dat", data_frame);
        appendTecplot("append.dat", data_frame);
        std::vector<std::string> expected = {R"(TITLE="Powered By Parfait")",
                                             R"(VARIABLES="Digimon cards", "Pokemon cards")",
                                             R"(ZONE T="append")",
                                             R"(1.000000 0.100000)",
                                             R"(2.000000 0.200000)",
                                             R"(3.000000 0.300000)",
                                             R"(1.000000 0.100000)",
                                             R"(2.000000 0.200000)",
                                             R"(3.000000 0.300000)"};
        auto actual = StringTools::split(FileTools::loadFileToString("append.dat"), "\n");
        REQUIRE(expected == actual);
        REQUIRE(0 == remove("append.dat"));
    }
}

TEST_CASE("Can sort a dataframe by a column") {
    DataFrame df;
    df["x"] = {3.0, 2.0, 6.0, -1.0};
    df["dog"] = {30.0, 20.0, 60.0, -10.0};
    df["bear"] = {-30.0, -20.0, -60.0, 10.0};

    df.sort("x");
    auto dog = df["dog"];
    REQUIRE(dog == std::vector<double>{-10, 20, 30, 60});

    auto bear = df["bear"];
    REQUIRE(bear == std::vector<double>{10.0, -20, -30, -60});
}

TEST_CASE("Get a sensible result for dataframe.shape() on an empty dataframe") {
    DataFrame df;
    auto shape = df.shape();
    REQUIRE(shape[0] == 0);
    REQUIRE(shape[1] == 0);
}
