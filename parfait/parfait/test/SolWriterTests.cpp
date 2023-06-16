
// These tests fail intermittently.  Disabling at Cameron's request.
#if 0
#include <RingAssertions.h>
#include <parfait/GammaIO.h>

using namespace Parfait;

TEST_CASE("read/write refine style metric"){
    REQUIRE_THROWS(SolReader("dog.dat"));

    SolWriter writer;

    long node_count = 7;
    writer.registerNodeField("metric",node_count,6);


    writer.open("dog.solb");
    std::vector<double> metric_a(3*6,1.0);
    std::vector<double> metric_b((node_count-3)*6,2.0);
    long starting_global_id = 0;
    writer.writeNodeField("metric",starting_global_id,metric_a);
    starting_global_id = 3;
    writer.writeNodeField("metric",starting_global_id,metric_b);
    writer.close();

    SolReader reader("dog.solb");
    REQUIRE(4 == reader.version());
    REQUIRE(3 == reader.dimensionality());

    REQUIRE(reader.has(GammaSolIO::Keywords::Dimensionality));
    REQUIRE(reader.has(GammaSolIO::Keywords::NodalSolution));
    REQUIRE(reader.has(GammaSolIO::Keywords::End));

    auto result_metric = reader.readNodeField("metric",0,node_count-1);
    reader.close();

    REQUIRE(metric_a.size()+metric_b.size() == result_metric.size());
    REQUIRE(1 == result_metric.front());
    REQUIRE(2 == result_metric.back());
}

TEST_CASE("read/write multiple fields as separate records"){
    int node_count = 7;

    SolWriter writer;
    writer.registerNodeField("density",node_count,1);
    writer.registerNodeField("pressure",node_count,1);
    writer.open("snap.solb");

    std::vector<double> a(3,1);
    std::vector<double> b(4,2);
    writer.writeNodeField("density",0,a);
    writer.writeNodeField("density",3,b);

    std::vector<double> d(3,3);
    std::vector<double> e(4,4);
    writer.writeNodeField("pressure",0,d);
    writer.writeNodeField("pressure",3,e);
    writer.close();

    SolReader reader("snap.solb");
    auto names = reader.nodeFieldNames();
    REQUIRE(2 == names.size());

    auto result = reader.readNodeField("density",0,node_count-1);
    REQUIRE(7 == result.size());
    REQUIRE(1 == result.front());
    REQUIRE(2 == result.back());

    result = reader.readNodeField("pressure",0,node_count-1);
    REQUIRE(3 == result.front());
    REQUIRE(4 == result.back());
}
#endif
