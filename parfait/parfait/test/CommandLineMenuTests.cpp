#include <RingAssertions.h>
#include <parfait/CommandLineMenu.h>
#include <parfait/CommandLine.h>

using namespace Parfait;

TEST_CASE("parse arguments for utilities") {
    std::vector<std::string> args{"--file", "/path/file.txt"};

    CommandLineMenu menu;
    menu.addFlag({"-b", "--burner"}, "This turns on the burner.");
    menu.addParameter({"f", "file"}, "Input filename", false);

    menu.parse(args);

    REQUIRE_FALSE(menu.has("-b"));

    SECTION("can access with or without dashes") {
        REQUIRE(menu.has("--file"));
        REQUIRE(menu.has("-f"));
        REQUIRE(menu.has("file"));
        REQUIRE(menu.has("f"));
    }

    SECTION("can get values with or without dashes") {
        REQUIRE("/path/file.txt" == menu.get("--file"));
        REQUIRE("/path/file.txt" == menu.get("file"));
    }
}

TEST_CASE("enforce required arguments") {
    std::vector<std::string> args{"-x"};

    CommandLineMenu menu;
    menu.addFlag({"-x"}, "cut");
    menu.addParameter({"-z"}, "zipper is required", true);

    REQUIRE_THROWS(menu.parse(args));

    SECTION("Able to turn off exceptions in quiet mode") {
        CommandLineMenu quiet_menu(true);
        REQUIRE_NOTHROW(quiet_menu.parse(args));
    }
}

TEST_CASE("parameters can optionally have a default value") {
    CommandLineMenu menu;
    menu.addParameter({"--path"}, "path to file", true, "./");

    REQUIRE(menu.parse(CommandLineMenu::Strings{}));
}

TEST_CASE("can have multiple args per parameter") {
    CommandLineMenu menu;
    menu.addParameter({"--grids"}, "1 or more grid filenames", true);

    menu.parse(CommandLineMenu::Strings{"--grids", "a.grid", "b.grid"});
    auto grids = menu.getAsVector("grids");
    REQUIRE(2 == grids.size());
    REQUIRE("a.grid" == grids[0]);
    REQUIRE("b.grid" == grids[1]);
}

TEST_CASE("don't skip an argument after multiple parameters") {
    CommandLineMenu menu;
    menu.addParameter({"--grids"}, "1 or more grid filenames", true);
    menu.addParameter({"-o"}, "output filename", true);

    REQUIRE_NOTHROW(menu.parse("--grids a.grid b.grid -o output-filename"));

    auto grids = menu.getAsVector("--grids");
    REQUIRE(2 == grids.size());
    REQUIRE("a.grid" == grids[0]);
    REQUIRE("b.grid" == grids[1]);

    auto output_filename = menu.get("-o");
    REQUIRE(output_filename == "output-filename");
}

TEST_CASE("Can mark options as hidden") {
    CommandLineMenu menu;
    menu.addFlag({"--be-angry"}, "It is easy to be angry");
    menu.addHiddenFlag({"--be-happy"}, "Being happy is a secret");
    menu.addHiddenParameter({"--the-answer"}, "Knowing how is the answer");

    SECTION("Options are hidden by default") {
        auto help_string = menu.to_string();
        REQUIRE_THAT(help_string, Contains("be-angry"));
        REQUIRE_THAT(help_string, not Contains("happy"));
        REQUIRE_THAT(help_string, not Contains("secret"));
    }

    SECTION("But you can explicitly ask to see hidden options") {
        auto help_string = menu.to_string_include_hidden();
        REQUIRE_THAT(help_string, Contains("be-angry"));
        REQUIRE_THAT(help_string, Contains("happy"));
        REQUIRE_THAT(help_string, Contains("secret"));
    }
}

TEST_CASE("getDouble returns a double") {
    CommandLineMenu menu;
    menu.addParameter({"--my-double"}, "my double", true);
    menu.parse("--my-double 0.1");
    double my_double = menu.getDouble("my-double");
    REQUIRE(my_double == 0.1);
}

TEST_CASE("is argument") {
    REQUIRE_FALSE(CommandLineMenu::isArgument("--pikachu"));
    REQUIRE_FALSE(CommandLineMenu::isArgument("-p"));
    REQUIRE(CommandLineMenu::isArgument("pika"));
    REQUIRE(CommandLineMenu::isArgument("pika-chu"));
    REQUIRE(CommandLineMenu::isArgument("-1"));
    REQUIRE(CommandLineMenu::isArgument("-.1"));
}

TEST_CASE("get a list of doubles") {
    CommandLineMenu menu;
    menu.addParameter({"--my-doubles"}, "my double list", true);
    menu.parse("--my-doubles 0.1 0.2 0.3");
    auto list = menu.getAsVector("my-doubles");
    REQUIRE(list.size() == 3);

    SECTION("can get them as doubles instead of strings"){
        auto actual = menu.getDoubles("my-doubles");
        std::vector<double> my_doubles {.1,.2,.3};
        CHECK(my_doubles == actual);
    }
}

TEST_CASE("get a list of integers with ranges"){
    CommandLineMenu menu;
    menu.addParameter({"--my-integers"}, "my int list", true);
    menu.parse("--my-integers 1:5,8,13 15:17 19,23");

    auto actual = menu.getInts("my-integers");
    std::vector<int> expected {1,2,3,4,5,8,13,15,16,17,19,23};
    REQUIRE(expected == actual);
}

TEST_CASE("Get quoted strings as single entry so options can have spaces in them") {
    CommandLineMenu menu;
    menu.addParameter({"--select"}, "a parameter we can ask for", true);
    menu.parse(std::vector<std::string>{"--select", "Total Pressure"});
    auto selected_parameter = menu.get("select");
    REQUIRE("Total Pressure" == selected_parameter);
}
