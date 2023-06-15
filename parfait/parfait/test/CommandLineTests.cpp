#include <RingAssertions.h>
#include <parfait/CommandLine.h>

TEST_CASE("pop arguments full") {
    std::vector<std::string> args = {"--first-flag", "0", "6", "--next-flag"};
    auto flag_args = Parfait::CommandLine::popArgumentMatching({"--first-flag", "-f"}, args);
    REQUIRE(flag_args.size() == 3);
    REQUIRE(flag_args[0] == "--first-flag");
    REQUIRE(flag_args[1] == "0");
    REQUIRE(flag_args[2] == "6");

    REQUIRE(args.size() == 1);
    REQUIRE(args[0] == "--next-flag");
}

TEST_CASE("pop arguments short") {
    std::vector<std::string> args = {"--this-better-be-here", "-f", "0", "6", "--next-flag"};
    auto flag_args = Parfait::CommandLine::popArgumentMatching({"--first-flag", "-f"}, args);
    REQUIRE(flag_args.size() == 3);
    REQUIRE(flag_args[0] == "-f");
    REQUIRE(flag_args[1] == "0");
    REQUIRE(flag_args[2] == "6");

    REQUIRE(args.size() == 2);
    REQUIRE(args[0] == "--this-better-be-here");
    REQUIRE(args[1] == "--next-flag");
}

TEST_CASE("Get all the original arguments if no flags match") {
    std::vector<std::string> args = {"-f", "0", "6", "--next-flag"};
    std::vector<std::string> expected_args = {"-f", "0", "6", "--next-flag"};

    auto flag_args = Parfait::CommandLine::popArgumentMatching({"--this-doesnt-match"}, args);

    REQUIRE(expected_args.size() == args.size());
    for (unsigned int i = 0; i < expected_args.size(); i++) {
        REQUIRE(expected_args[i] == args[i]);
    }
}
