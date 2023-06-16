#include <RingAssertions.h>
#include <parfait/BarChart.h>

using namespace Parfait;

TEST_CASE("bucketize data into histogram") {
    std::vector<int> data{0, 3, 4, 4, 5, 5, 5, 6, 9, 9, 9, 9, 9, 10};
    int nbuckets = 11;
    BarChart chart(data);
    auto histogram_counts = chart.generateHistogramCounts(nbuckets);
    REQUIRE(nbuckets == int(histogram_counts.size()));
    REQUIRE(1 == histogram_counts[0]);
    REQUIRE(0 == histogram_counts[1]);
    REQUIRE(2 == histogram_counts[4]);
    REQUIRE(5 == histogram_counts[9]);
    REQUIRE(1 == histogram_counts[10]);

#if 0
    auto s = chart.generateHistogram(nbuckets,BarGenerators::Utf8BarGenerator());
    printf("SparkBarChart: <<%s>>\n",s.c_str());

    auto generator = BarGenerators::HorizontalBarGenerator();
    generator.setMaxWidth(40);
    generator.setPlotSymbol("*");
    generator.setLeftBoundSymbol("[");
    generator.setRightBoundSymbol("]");
    s = chart.generateHistogram(nbuckets,generator);
    printf("Horizontal:\n%s\n",s.c_str());
#endif
}
