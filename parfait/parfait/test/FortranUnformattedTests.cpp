#include <RingAssertions.h>
#include <parfait/StringTools.h>
#include <parfait/FortranUnformatted.h>

using namespace Parfait;

TEST_CASE("Can read/write fortran unformatted files") {
    auto filename = StringTools::randomLetters(5) + "-";
    std::vector<double> values = {1, 2, 3, 4};
    {
        auto f = fopen(filename.c_str(), "w");
        FortranUnformatted::writeMarker<double>(2, f);
        fwrite(values.data(), sizeof(double), 2, f);
        FortranUnformatted::writeMarker<double>(2, f);
        FortranUnformatted::writeMarker<double>(values.size(), f);
        fwrite(values.data(), sizeof(double), values.size(), f);
        FortranUnformatted::writeMarker<double>(values.size(), f);
        fclose(f);
    }
    {
        auto f = fopen(filename.c_str(), "r");
        FortranUnformatted::skipRecord(f);
        auto marker = FortranUnformatted::readMarker(f);
        REQUIRE(int(values.size() * sizeof(double)) == marker);
        for (double expected : values) {
            double actual;
            fread(&actual, sizeof(double), 1, f);
            REQUIRE(expected == actual);
        }
        REQUIRE_NOTHROW(FortranUnformatted::checkMarker(marker, f));
    }
}