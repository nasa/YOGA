#include <random>
#include <RingAssertions.h>
#include <TracerImpl.h>
#include <Tracer.h>

std::string randomLetters(int n) {
    std::string s(n, 'a');
    std::random_device rd;
    srand(rd());
    std::transform(s.begin(), s.end(), s.begin(), [](char a) { return a + rand() % 26; });
    return s;
}

bool doesFileExist(std::string filename) {
    bool good = false;
    FILE* fp = fopen(filename.c_str(), "r");
    if (fp == nullptr) {
        good = false;
    } else {
        good = true;
        fclose(fp);
    }
    return good;
}

std::string loadFileToString(std::string filename) {
    std::ifstream f(filename.c_str());
    if (not f.is_open()) {
        throw std::logic_error("Could not open file for reading: " + filename);
    }
    auto contents = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    f.close();
    return contents;
}

namespace MyNamespace {
void SpecialFunction() {
    TRACER_FUNCTION_BEGIN
    TRACER_FUNCTION_END
}
}

TEST_CASE("Tracer constructor sets necessary bits") {
    auto filename = randomLetters(7) + ".trace";
    auto tracer = std::make_shared<TracerImpl>(filename, 1);
    REQUIRE(filename == tracer->file_name);
    REQUIRE(1 == tracer->process_id);
    REQUIRE(0 == remove(filename.c_str()));
}

TEST_CASE("Tracer can open and close a trace") {
    auto filename = randomLetters(7) + ".trace";
    auto tracer = std::make_shared<TracerImpl>(filename, 0);

    SECTION("Empty file has open close braces") {
        tracer.reset();
        tracer = nullptr;
        auto contents = loadFileToString(filename);

        REQUIRE_THAT(contents, StartsWith("["));
        REQUIRE_THAT(contents, EndsWith("]"));
    }

    SECTION("begin event creates a begin statement") {
        tracer->begin("begin-event", "my-category");

        tracer.reset();
        tracer = nullptr;
        auto contents = loadFileToString(filename);

        REQUIRE_THAT(contents, Contains("begin-event"));
        REQUIRE_THAT(contents, Contains("my-category"));
    }

    REQUIRE(0 == remove(filename.c_str()));
}

TEST_CASE("Tracer global initialize creates a trace file") {
    auto filename = randomLetters(7) + ".trace";
    Tracer::initialize(filename);
    MyNamespace::SpecialFunction();
    Tracer::finalize();
    printf("expecting filename: %s\n", filename.c_str());
    auto contents = loadFileToString(filename);
    REQUIRE_THAT(contents, Contains("SpecialFunction"));
    REQUIRE(0 == remove(filename.c_str()));
}

TEST_CASE("Tracer global functions don't work if tracer isn't initialized") {
    Tracer::begin("monkey");
    Tracer::flush();

    REQUIRE_FALSE(doesFileExist("trace.trace"));
}
