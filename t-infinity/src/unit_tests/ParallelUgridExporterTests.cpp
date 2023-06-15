#include <RingAssertions.h>
#include <t-infinity/ParallelUgridExporter.h>
#include <t-infinity/CartMesh.h>
#include <parfait/FileTools.h>
#include <parfait/MapbcParser.h>

using namespace inf;
class UgridExporterTester : public ParallelUgridExporter {
  public:
    static std::string modify(const std::string filename) { return setUgridExtension(filename); }
};

TEST_CASE("Make a grid and write a mapbc") {
    auto mp = MessagePasser(MPI_COMM_WORLD);
    auto mesh = inf::CartMesh::create(mp, 5, 5, 5);
    mesh->setTagName(1, "Dog1");
    mesh->setTagName(2, "Fox2");
    auto name = Parfait::StringTools::randomLetters(12);
    mp.Broadcast(name, 0);
    inf::ParallelUgridExporter::write(name, *mesh, mp);
    REQUIRE(Parfait::FileTools::doesFileExist(name+".lb8.ugrid"));
    auto mapbc_name = name + ".mapbc";
    Parfait::FileTools::waitForFile(mapbc_name, 30);
    INFO("mapbc filename: " + mapbc_name);
    REQUIRE(Parfait::FileTools::doesFileExist(mapbc_name));
    auto contents = Parfait::FileTools::loadFileToString(mapbc_name);
    auto bc_map = Parfait::MapbcParser::parse(contents);
    REQUIRE(bc_map.size() == (size_t)6);
    REQUIRE(bc_map.at(1).second == "Dog1");
    REQUIRE(bc_map.at(2).second == "Fox2");
    REQUIRE(bc_map.at(3).second == "Tag_3");
    REQUIRE(bc_map.at(4).second == "Tag_4");
    REQUIRE(bc_map.at(5).second == "Tag_5");
    REQUIRE(bc_map.at(6).second == "Tag_6");
}

TEST_CASE("Select endiannes based on filename") {
    SECTION("If the filename indicates big/little endian, leave as is") {
        std::string name = "pikachu.b8.ugrid";
        name = UgridExporterTester::modify(name);
        REQUIRE("pikachu.b8.ugrid" == name);

        name = "pikachu.lb8.ugrid";
        name = UgridExporterTester::modify(name);
        REQUIRE("pikachu.lb8.ugrid" == name);
    }

    SECTION("If endiannes not indicated by the name, default to little") {
        std::string name = "pikachu";
        name = UgridExporterTester::modify(name);
        REQUIRE("pikachu.lb8.ugrid" == name);
    }
}