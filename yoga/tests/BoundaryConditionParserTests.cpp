#include <RingAssertions.h>
#include <string>
#include <parfait/StringTools.h>
#include <BoundaryConditionParser.h>

using namespace YOGA;

TEST_CASE("parse an empty string"){
    auto domain_boundaries = BoundaryInfoParser::parse("");
    REQUIRE(domain_boundaries.empty());
}

TEST_CASE("get solid tags for boundary"){
    std::string s = "domain fuselage "
                    "solid 1 2 3 ";

    auto domain_boundaries = BoundaryInfoParser::parse(s);

    REQUIRE(1 == domain_boundaries.size());
    REQUIRE("fuselage" == domain_boundaries[0].name);
    REQUIRE(domain_boundaries[0].x_symmetry_tags.empty());
    REQUIRE(domain_boundaries[0].y_symmetry_tags.empty());
    REQUIRE(domain_boundaries[0].z_symmetry_tags.empty());
    REQUIRE(domain_boundaries[0].interpolation_tags.empty());
    REQUIRE(domain_boundaries[0].solid_wall_tags == std::vector<int>{1,2,3});
}

TEST_CASE("can input ranges of tags when consecutive"){
    std::string s = "domain fuselage "
        "solid 1:3 7 9:12";

    auto domain_boundaries = BoundaryInfoParser::parse(s);

    REQUIRE(1 == domain_boundaries.size());
    REQUIRE("fuselage" == domain_boundaries[0].name);
    REQUIRE(domain_boundaries[0].solid_wall_tags == std::vector<int>{1,2,3,7,9,10,11,12});

}

TEST_CASE("parse a domain with no relevant boundary conditions"){
    std::string s = "domain box";
    auto domain_boundaries = BoundaryInfoParser::parse(s);
    REQUIRE(1 == domain_boundaries.size());
    REQUIRE("box" == domain_boundaries[0].name);
    REQUIRE(domain_boundaries[0].interpolation_tags.empty());
    REQUIRE(domain_boundaries[0].z_symmetry_tags.empty());
    REQUIRE(domain_boundaries[0].y_symmetry_tags.empty());
    REQUIRE(domain_boundaries[0].x_symmetry_tags.empty());
    REQUIRE(domain_boundaries[0].solid_wall_tags.empty());
}

TEST_CASE("fail with useful error if input is wrong"){
    std::string s = "dimain fuselage solid 1 2";
    REQUIRE_THROWS(BoundaryInfoParser::parse(s));
}

TEST_CASE("allow setting importance levels for components"){
    std::string s =
        "domain box "
        "importance 1 "
        "domain smaller-box "
        "importance 2 ";

    auto domain_boundaries = BoundaryInfoParser::parse(s);
    REQUIRE(domain_boundaries.size() == 2);

}

TEST_CASE("handle multiple domains that share tag info") {
    std::string s =
        "domain fuselage "
        "solid 1 2 3 "
        "z-symmetry 4 "
        "x-symmetry 5 "
        "y-symmetry 6 "
        "domain blade-1 blade-2 blade-3 blade_4 "
        "solid 1 2 "
        "interpolation -1";

    auto domain_boundaries = BoundaryInfoParser::parse(s);
    REQUIRE(5 == domain_boundaries.size());
    REQUIRE(std::vector<int>{1,2,3} == domain_boundaries[0].solid_wall_tags);
    REQUIRE(std::vector<int>{4} == domain_boundaries[0].z_symmetry_tags);
    REQUIRE(std::vector<int>{5} == domain_boundaries[0].x_symmetry_tags);
     REQUIRE(std::vector<int>{6} == domain_boundaries[0].y_symmetry_tags);

    REQUIRE(std::vector<int>{1,2} == domain_boundaries[1].solid_wall_tags);
    REQUIRE(std::vector<int>{1,2} == domain_boundaries[2].solid_wall_tags);
    REQUIRE(std::vector<int>{1,2} == domain_boundaries[3].solid_wall_tags);
    REQUIRE(std::vector<int>{1,2} == domain_boundaries[4].solid_wall_tags);

    REQUIRE(std::vector<int>{-1} == domain_boundaries[1].interpolation_tags);
}
