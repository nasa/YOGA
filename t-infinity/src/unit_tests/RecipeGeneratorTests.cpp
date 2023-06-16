#include <RingAssertions.h>
#include <string>
#include <parfait/StringTools.h>
#include <t-infinity/RecipeGenerator.h>

using namespace inf;

void requireFilenamesExtracted(std::string script, const std::vector<std::string>& filenames) {
    auto recipes = RecipeGenerator::parse(script);
    REQUIRE(filenames.size() == recipes.size());
    for (size_t i = 0; i < filenames.size(); i++) REQUIRE(filenames[i] == recipes[i].filename);
}

void requirePluginsExtracted(std::string script, const std::vector<std::string>& plugins) {
    auto recipes = RecipeGenerator::parse(script);
    REQUIRE(plugins.size() == recipes.size());
    for (size_t i = 0; i < plugins.size(); i++) REQUIRE(plugins[i] == recipes[i].plugin);
}

TEST_CASE("an empty script should yield no recipes") {
    auto recipes = RecipeGenerator::parse("");
    REQUIRE(recipes.size() == 0);
}

TEST_CASE("if a file is saved, it can be named") {
    requireFilenamesExtracted("save file pikachu", {"pikachu"});
    requireFilenamesExtracted("save charmander ", {"charmander"});
    requireFilenamesExtracted("save as bulbasaur ", {"bulbasaur"});
    requireFilenamesExtracted("save flow.snap", {"flow.snap"});
}

TEST_CASE("should be able to save multiple files") {
    std::string command = "save file pikachu";
    command += "\nsave file charmander";

    requireFilenamesExtracted(command, {"pikachu", "charmander"});
}

TEST_CASE("should skip comment lines") {
    auto recipes = RecipeGenerator::parse("# This is a comment");
    REQUIRE(recipes.size() == 0);
    requireFilenamesExtracted("#comment line\nsave file pikachu", {"pikachu"});
    requireFilenamesExtracted("#comment line\nsave file\n\tpikachu", {"pikachu"});
}

TEST_CASE("default plugin should be parfait") {
    std::string command = "save as pikachu.vtk";
    command += "\nset plugin refine\n";
    command += "save file charmander.tec";

    requireFilenamesExtracted(command, {"pikachu.vtk", "charmander.tec"});
    requirePluginsExtracted(command, {"ParfaitViz", "refine"});
}

TEST_CASE("should throw if the first word is not a command") {
    REQUIRE_THROWS(RecipeGenerator::parse("Invalid command example"));
}

void requireFieldNamesExtracted(const std::string& command, const std::vector<std::string>& fields) {
    auto recipes = RecipeGenerator::parse(command);
    REQUIRE(recipes.size() == 1);
    auto recipe = recipes.front();
    REQUIRE(fields.size() == recipe.fields.size());
    for (size_t i = 0; i < fields.size(); i++) REQUIRE(fields[i] == recipe.fields[i]);
}


TEST_CASE("should be able to select fields"){
    std::string command = "set plugin snap\n";
    command += "select fields density x-momentum y-momentum total-energy\n";
    command += " save as meanflow";

    requireFilenamesExtracted(command, {"meanflow"});
    requireFieldNamesExtracted(command, {"density", "x-momentum", "y-momentum", "total-energy"});
}

TEST_CASE("should be able to select a single field without pluralization"){
    std::string command = "set plugin snap\n";
    command += "select field density\n";
    command += "save as meanflow";

    requireFieldNamesExtracted(command,{"density"});
}

void requireFilterTypes(const std::string& command, const std::vector<std::string>& types) {
    auto recipes = RecipeGenerator::parse(command);
    REQUIRE(recipes.front().filters.size() == types.size());
    for (size_t i = 0; i < types.size(); i++) REQUIRE(recipes.front().filters[i]->type() == types[i]);
}

TEST_CASE("should be able to select surfaces") {
    std::string command = "select surfaces";
    command += "\n    save as pikachu";

    requireFilenamesExtracted(command, {"pikachu"});
    requireFilterTypes(command, {"surface-filter"});
}

TEST_CASE("should be able to select volume elements") {
    std::string command = "select volume";
    command += "\n    save as pikachu";

    requireFilenamesExtracted(command, {"pikachu"});
    requireFilterTypes(command, {"volume-filter"});
}

#if 0
void requireTags(const std::string& command, const std::set<int>& tags) {
    auto recipes = Vista::parse(command);
    auto recipe = recipes.front();
    bool found_tags = false;
    for (auto filter : recipe.filters) {
        if ("tag-filter" == filter->type()) {
            Vista::TagFilterRecipe* tag_filter = (Vista::TagFilterRecipe*)filter.get();
            auto extracted_tags = tag_filter->getTags();
            REQUIRE(extracted_tags.size() == tags.size());
            for (auto tag : tags) REQUIRE(extracted_tags.count(tag) == 1);
            found_tags = true;
        }
    }
    REQUIRE(found_tags);
}

TEST_CASE("should be able to select elements by tags") {
    std::string command = "select tags 1 2\n";
    command += "    save file charmander";

    requireFilenamesExtracted(command, {"charmander"});
    requireFilterTypes(command, {"tag-filter"});

    std::set<int> expected_tags{1, 2};
    requireTags(command, expected_tags);
}

TEST_CASE("should be allow a range of tags") {
    std::set<int> one_through_six{1, 2, 3, 4, 5, 6};

    SECTION("select range with braces") {
        std::string command = "select tags [1 6]\n";
        command += "    save file charmander";

        requireFilenamesExtracted(command, {"charmander"});
        requireFilterTypes(command, {"tag-filter"});

        requireTags(command, one_through_six);
    }

    SECTION("allow space on opening brace") {
        std::string command = "select tags [ 1 6]\n";
        command += "    save file charmander";

        requireTags(command, one_through_six);
    }

    SECTION("allow space on closing brace") {
        std::string command = "select tags [1 6 ]\n";
        command += "    save file charmander";

        requireTags(command, one_through_six);
    }

    SECTION("select individual numbers and range") {
        std::string command = "select tags [1 3] 4 [5 6 ]\n";
        command += "    save file charmander";

        requireTags(command, one_through_six);
    }
}

TEST_CASE("should be able to combine surface and tag selection") {
    std::string command = "select surfaces and select tags 1 2\n";
    command += "    save file charmander";

    requireFilenamesExtracted(command, {"charmander"});
    requireFilterTypes(command, {"surface-filter", "tag-filter"});
    requireTags(command, {1, 2});
}

TEST_CASE("should be able to specify which fields should be plotted") {
    std::string command = "save file pikachu with density\n";

    requireFilenamesExtracted(command, {"pikachu"});
    requireFieldNamesExtracted(command, {"density"});
}

TEST_CASE("should be able to select multiple fields") {
    std::string command = "save file pikachu with density pressure x-momentum\n";

    requireFilenamesExtracted(command, {"pikachu"});
    requireFieldNamesExtracted(command, {"density", "pressure", "x-momentum"});
}

void requireSliceLocations(const std::string& command, const std::vector<double>& locations) {
    auto recipes = Vista::parse(command);
    auto recipe = recipes.front();
    int i = 0;
    for (auto filter : recipe.filters) {
        bool is_slice = filter->type().find("-slice") != std::string::npos;
        if (is_slice) {
            Vista::SliceFilterRecipe* tag_filter = (Vista::SliceFilterRecipe*)filter.get();
            double x = tag_filter->getLocation();
            REQUIRE(locations[i++] == Approx(x));
        }
    }
    REQUIRE(i == locations.size());
}

TEST_CASE("should be able to create slices") {
    SECTION("slice in x") {
        std::string command = "create x slice at 0.5 save file pikachu";

        requireFilenamesExtracted(command, {"pikachu"});
        requireFilterTypes(command, {"x-slice"});
        requireSliceLocations(command, {0.5});
    }

    SECTION("slice in y") {
        std::string command = "create y slice at 0.3 save file pikachu";

        requireFilterTypes(command, {"y-slice"});
        requireSliceLocations(command, {0.3});
    }

    SECTION("slice in z") {
        std::string command = "create z slice at 0.2 save file pikachu";

        requireFilterTypes(command, {"z-slice"});
        requireSliceLocations(command, {0.2});
    }
}

TEST_CASE("should handle a combination of recipes") {
    std::string command = "select surfaces and create x slice at 0.7\n";
    command += "save as pikachu with resistance energy-units mass\n";
    command += "create z slice at 1.5 and select volume\n";
    command += "and select tags 1 2 [5 7] 9\n";
    command += "save charmander with mass";

    requireFilenamesExtracted(command, {"pikachu", "charmander"});

    auto recipes = Vista::parse(command);
    auto recipe = recipes.front();
    REQUIRE("surface-filter" == recipe.filters.front()->type());
    REQUIRE("x-slice" == recipe.filters.back()->type());

    auto selector = recipe.getCellSelector();
}
#endif