#include <t-infinity/MotionMatrixParser.h>
#include <RingAssertions.h>

using namespace inf;

TEST_CASE("empty config") {
    std::string config = "";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 0);
}

TEST_CASE("throw if there's something unexpected in the file"){
    std::string config = "grid pika.ugrid\n";
    config.append("uncle bob\n");
    REQUIRE_THROWS(MotionParser::parse(config));
}

TEST_CASE("specify one grid") {
    std::string config = "grid pika.ugrid";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 1);
}
TEST_CASE("specify one grid with tab") {
    std::string config = "grid\tchu.ugrid";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 1);
    REQUIRE(entries.front().grid_name == "chu.ugrid");
}
TEST_CASE("specify two grids") {
    std::string config = "domain pika-1-domain grid pikachu.ugrid\ngrid charmander.ugrid";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 2);
    REQUIRE(entries[0].grid_name == "pikachu.ugrid");
    REQUIRE(entries[1].grid_name == "charmander.ugrid");
}
TEST_CASE("specify reflection for grid"){
    std::string config = "grid pika.ugrid";
    config += "\nreflect 0 0 1 2";
    auto entries = MotionParser::parse(config);
    REQUIRE(1 == entries.size());
    auto entry = entries.front();
    REQUIRE(entry.has_reflection);
    Parfait::Point<double> expected_plane_normal {0,0,1};
    double expected_plane_offset = 2.0;
    REQUIRE(expected_plane_normal.approxEqual(entry.plane_normal));
    REQUIRE(expected_plane_offset == entry.plane_offset);
}
TEST_CASE("grid with translation") {
    std::string config = "grid pika.ugrid\ntranslate .1 0 0\n";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 1);
    auto entry = entries[0];
    REQUIRE(entry.grid_name == "pika.ugrid");
    Parfait::Point<double> p{1, 0, 0};
    entry.motion_matrix.movePoint(p.data());
    REQUIRE(p[0] == 1.1);
    REQUIRE(p[1] == 0.0);
    REQUIRE(p[2] == 0.0);
}
TEST_CASE("parse decimals with no leading zero"){
    std::string config = "grid pika.ugrid\ntranslate 1 0 0\n";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 1);
    auto entry = entries[0];
    REQUIRE(entry.grid_name == "pika.ugrid");
    Parfait::Point<double> p{1, 0, 0};
    entry.motion_matrix.movePoint(p.data());
    REQUIRE(p[0] == 2.0);
    REQUIRE(p[1] == 0.0);
    REQUIRE(p[2] == 0.0);

}

TEST_CASE("two grids with translations") {
    std::string config = "grid pika.ugrid\ntranslate 1 0 0\ngrid chu.ugrid translate 2 0 0";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 2);
    auto entry = entries.at(1);
    REQUIRE(entry.grid_name == "chu.ugrid");
    Parfait::Point<double> p{1, 0, 0};
    entry.motion_matrix.movePoint(p.data());
    REQUIRE(p[0] == 3.0);
    REQUIRE(p[1] == 0.0);
    REQUIRE(p[2] == 0.0);
}
TEST_CASE("grid with rotation") {
    std::string config = "grid pika.ugrid\nrotate 90 from 0 0 0 to 0 0 1\n";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 1);
    auto entry = entries[0];
    REQUIRE(entry.grid_name == "pika.ugrid");
    Parfait::Point<double> p{0, 1, 0};
    entry.motion_matrix.movePoint(p.data());
    REQUIRE(p[0] == Approx(-1.0).margin(1e-8));
    REQUIRE(p[1] == Approx(0.0).margin(1e-8));
    REQUIRE(p[2] == Approx(0.0).margin(1e-8));
}

TEST_CASE("Specify motion in 4x4 matrix format"){
    std::string config = "grid pika.ugrid\nmove\n";
    config += "1 .1 .1 .1\n";
    config += "0 1 0 0\n";
    config += "0 0 1 0\n";
    config += "0 0 0 1\n";

    std::vector<double> expected {1,.1,.1,.1,0,1,0,0,0,0,1,0,0,0,0,1};
    std::vector<double> actual(16,0.0);

    auto entries = MotionParser::parse(config);
    auto entry = entries.front();
    entry.motion_matrix.getMatrix(actual.data());
    REQUIRE(expected == actual);
}

TEST_CASE("full monty") {
    std::string config =
        "# this is a comment grid dudeman.grid\n"
        "grid pika.ugrid "
        "rotate 90 "
        "from 0 0 0 "
        "to 0 0 1 \n"
        "# comment again! because we can\n"
        "grid charmander.ugrid "
        "grid chu.ugrid "
        "translate 1 1 1 "
        "rotate 180 "
        "from 0 0 0 "
        "to 0 0 1 ";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 3);
}
TEST_CASE("optionally combine mapbc files") {
    std::string config =
        "# this is a comment grid dudeman.grid\n"
        "grid pika.ugrid "
        "mapbc pika.mapbc "
        "rotate 90 "
        "from 0 0 0 "
        "to 0 0 1 \n"
        "# comment again! because we can\n"
        "grid charmander.ugrid "
        "mapbc charmander.mapbc "
        "grid chu.ugrid "
        "mapbc chu.mapbc "
        "translate 1 1 1 "
        "rotate 180 "
        "from 0 0 0 "
        "to 0 0 1 ";
    auto entries = MotionParser::parse(config);
    REQUIRE(entries.size() == 3);
    REQUIRE(entries[0].mapbc_name == "pika.mapbc");
}
