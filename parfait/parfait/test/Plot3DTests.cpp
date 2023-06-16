#include <RingAssertions.h>
#include <parfait/Plot3DReader.h>
#include <parfait/Plot3DWriter.h>
#include <MessagePasser/MessagePasser.h>
#include <parfait/StringTools.h>

TEST_CASE("Can write/read a Plot3D solution") {
    auto plot3d_writer = Parfait::Plot3DSolutionWriter();
    plot3d_writer.insert(0, 0, 0, 0, 0, 1.25);
    auto filename = Parfait::StringTools::randomLetters(6) + "test.q";
    plot3d_writer.write(filename);

    auto plot3d_reader = Parfait::Plot3DReader(filename);
    auto v = plot3d_reader.getVariable(0, 0, 0, 0, 0);
    REQUIRE(1.25 == Approx(v));
    REQUIRE(0 == remove(filename.c_str()));
}

TEST_CASE("Can write/read a Plot3D mesh") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    if (mp.NumberOfProcesses() != 1) return;
    auto plot3d_writer = Parfait::Plot3DGridWriter();
    REQUIRE(0 == plot3d_writer.blockCount());

    int number_of_blocks = 2;
    auto insertPoint = [&](int i, int j, int k) {
        auto x = double(i);
        auto y = double(j);
        auto z = double(k);
        plot3d_writer.setPoint(0, i, j, k, {x, y, z});
        plot3d_writer.setPoint(0, i + 1, j, k, {x + 1, y, z});
        plot3d_writer.setPoint(1, i, j, k, {x + 2, y, z});
        plot3d_writer.setPoint(1, i + 1, j, k, {x + 3, y, z});
    };
    insertPoint(0, 0, 0);
    insertPoint(1, 0, 0);
    insertPoint(1, 1, 0);
    insertPoint(0, 1, 0);
    insertPoint(0, 0, 1);
    insertPoint(1, 0, 1);
    insertPoint(1, 1, 1);
    insertPoint(0, 1, 1);

    plot3d_writer.write("test.g");
    auto plot3d_reader = Parfait::Plot3DGridReader("test.g");

    for (int block_id = 0; block_id < number_of_blocks; ++block_id) {
        REQUIRE(number_of_blocks == plot3d_writer.blockCount());
        auto block_dimensions = plot3d_writer.blockDimensions(block_id);
        REQUIRE(3 == block_dimensions.i);
        REQUIRE(2 == block_dimensions.j);
        REQUIRE(2 == block_dimensions.k);

        REQUIRE(number_of_blocks == plot3d_reader.blockCount());
        REQUIRE(block_dimensions.i == plot3d_reader.blockDimensions(block_id).i);
        REQUIRE(block_dimensions.j == plot3d_reader.blockDimensions(block_id).j);
        REQUIRE(block_dimensions.k == plot3d_reader.blockDimensions(block_id).k);

        for (int k = 0; k < block_dimensions.k; ++k) {
            for (int j = 0; j < block_dimensions.j; ++j) {
                for (int i = 0; i < block_dimensions.i; ++i) {
                    auto expected_point = plot3d_writer.getPoint(block_id, i, j, k);
                    auto actual_point = plot3d_reader.getXYZ(block_id, i, j, k);
                    INFO("i: " + std::to_string(i) + " j: " + std::to_string(j) + " k: " + std::to_string(k));
                    INFO("actual:   " + actual_point.to_string());
                    INFO("expected: " + expected_point.to_string());
                    REQUIRE(expected_point[0] == Approx(actual_point[0]).margin(1e-15));
                    REQUIRE(expected_point[1] == Approx(actual_point[1]).margin(1e-15));
                    REQUIRE(expected_point[2] == Approx(actual_point[2]).margin(1e-15));
                }
            }
        }
    }
    REQUIRE(0 == remove("test.g"));
}

TEST_CASE("Plot3DWriter throws for missing data") {
    auto plot3d_writer = Parfait::Plot3DSolutionWriter();
    plot3d_writer.insert(0, 1, 0, 0, 0, 1.0);
    REQUIRE_THROWS_WITH(
        plot3d_writer.write("dummy"),
        "Error:non-contiguous data.  Verify that insert calls span the full space the structured grid.");
}