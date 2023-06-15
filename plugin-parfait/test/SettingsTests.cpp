#include <RingAssertions.h>
#include "../shared/JsonSettings.h"

TEST_CASE("Pre-processor can use json formatted string, or a filename") {
    SECTION("Just a grid filename") {
        std::string input_string = "my_grid.b8.ugrid";
        auto settings = PluginParfait::parseJsonSettings(input_string);
        REQUIRE(settings.at("grid_filename").asString() == "my_grid.b8.ugrid");
    }

    SECTION("A JSON formatted string") {
        std::string input_string = R"({"grid_filename":"my_grid.b8.ugrid"})";
        auto settings = PluginParfait::parseJsonSettings(input_string);
        REQUIRE(settings.at("grid_filename").asString() == "my_grid.b8.ugrid");
    }
}
