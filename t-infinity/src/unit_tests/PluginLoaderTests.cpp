#include <RingAssertions.h>
#include <t-infinity/PluginLoader.h>
#include <parfait/StringTools.h>

using namespace inf ::plugins;


bool contains(const std::vector<std::string>& names,const std::string& name){
    return 1 == std::count(names.begin(),names.end(),name);
}

TEST_CASE("get list of potential library names"){
    std::string base = "pikachu";
    auto names = getPossibleLibraryNames(base);

    for(auto& e:possibleExtensions()) {
        REQUIRE(contains(names, "pikachu"+e));
        REQUIRE(contains(names,"libpikachu"+e));
    }
}

void touch(const std::string& fake_library){
    Parfait::FileTools::writeStringToFile(fake_library,"stuff");
    Parfait::FileTools::waitForFile(fake_library,5);
}


TEST_CASE("find actual library with given base name"){
    SECTION("can find libs without lib prefix") {
        std::string expected_lib = "pikachu.so";
        touch(expected_lib);
        auto actual_lib = findLibraryWithBasename("./", "pikachu");
        REQUIRE(expected_lib == actual_lib);
    }
    SECTION("can find libs with lib prefix"){
        std::string expected_lib = "libcharmander.so";
        touch(expected_lib);
        auto actual_lib = findLibraryWithBasename("./", "charmander");
        REQUIRE(expected_lib == actual_lib);
    }

    #if __APPLE__
    SECTION("find dylibs without prefix"){
        std::string expected_lib = "bulbasaur.dylib";
        touch(expected_lib);
        auto actual_lib = findLibraryWithBasename("./", "bulbasaur");
        REQUIRE(expected_lib == actual_lib);
    }
    SECTION("also find dylibs with prefix"){
        std::string expected_lib = "libsquirtle.dylib";
        touch(expected_lib);
        auto actual_lib = findLibraryWithBasename("./", "squirtle");
        REQUIRE(expected_lib == actual_lib);
    }
    #endif
}