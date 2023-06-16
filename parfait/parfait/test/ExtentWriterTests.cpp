#include <RingAssertions.h>
#include <parfait/Extent.h>
#include <parfait/ExtentWriter.h>

TEST_CASE("Extent writer") {
    std::vector<Parfait::Extent<double>> extents;
    for (int i = 0; i < 10; i++) {
        auto x = (double)i;
        auto e = Parfait::Extent<double>{{x, x, x}, {x + 1, x + 1, x + 1}};
        extents.push_back(e);
    }

    std::vector<double> data(10);
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = double(i);
    }

    Parfait::ExtentWriter::write("extents-no-data", extents);
    Parfait::ExtentWriter::write("extents-with-data", extents, data);
}
