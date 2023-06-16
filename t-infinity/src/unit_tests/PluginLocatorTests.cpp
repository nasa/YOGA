#include <RingAssertions.h>
#include <t-infinity/PluginLocator.h>
#include <parfait/StringTools.h>

TEST_CASE("get path to plugin dir"){
    auto path = inf::getPluginDir();
    printf("Plugin path: %s\n",path.c_str());
    REQUIRE(Parfait::StringTools::stripPath(path) == "lib");
}
