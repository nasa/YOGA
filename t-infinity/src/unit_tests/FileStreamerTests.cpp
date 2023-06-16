#include <RingAssertions.h>
#include <parfait/StringTools.h>
#include <parfait/FileTools.h>
#include <t-infinity/FileStreamer.h>

TEST_CASE("Simple file streamer can read and write ints") {
    auto writer = inf::FileStreamer::create("simple");
    auto hash = Parfait::StringTools::randomLetters(5);
    std::string filename = "simple.test." + hash;
    REQUIRE(writer->openWrite(filename));
    std::vector<int> threes(100, 3);
    writer->write(threes.data(), sizeof(int), threes.size());
    writer->close();

    Parfait::FileTools::waitForFile(filename, 10);

    auto reader = inf::FileStreamer::create("simple");
    REQUIRE(reader->openRead(filename));
    std::vector<int> maybe_threes(100, -1);
    reader->read(maybe_threes.data(), sizeof(int), maybe_threes.size());
    REQUIRE(maybe_threes == threes);
}