#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>
#include <parfait/MatrixMarketReader.h>
#include <parfait/LinearPartitioner.h>
#include <parfait/StringTools.h>

TEST_CASE("Matrix market reader", "[mmr]") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    std::string matrix_filename = MATRIX_DIR;
    matrix_filename += "test-matrix.mtx";

    int block_size = 7;
    MatrixMarketReader reader(matrix_filename, block_size);
    int num_rows = reader.numBlockRows();
    REQUIRE(num_rows == 129);

    auto range = Parfait::LinearPartitioner::getRangeForWorker(mp.Rank(), num_rows, mp.NumberOfProcesses());

    std::vector<std::set<long>> graph(range.end - range.start);

    auto fill_graph = [&](long block_row, long block_column, int equation_row, int equation_col, double entry) {
        if (range.owns(block_row)) {
            int local_id = block_row - range.start;
            graph.at(local_id).insert(block_column);
        }
    };
    reader.readAllEntries(fill_graph);

    REQUIRE((int)graph.size() == (range.end - range.start));
    if (mp.Rank() == 0) {
        REQUIRE(graph[0].count(0) == 1);
    }
}
