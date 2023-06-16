#pragma once
#include "CellSelector.h"
#include "PluginLocator.h"
#include <parfait/StringTools.h>
#include <parfait/Lexer.h>
#include <t-infinity/FilterFactory.h>
#include <queue>

namespace inf {

class RecipeGenerator {
  public:
    class FilterRecipe {
      public:
        virtual std::string type() = 0;
        virtual std::shared_ptr<CellSelector> getCellSelector() = 0;
        virtual ~FilterRecipe() = default;
    };

    class Recipe {
      public:
        std::shared_ptr<CellSelector> getCellSelector() {
            auto selector = std::make_shared<CompositeSelector>();
            for (auto& recipe : filters) selector->add(recipe->getCellSelector());
            return selector;
        }
        std::string plugin = "ParfaitViz";
        std::string filename = "unset";
        std::string plugin_directory = getPluginDir();
        std::vector<std::string> fields;
        std::vector<std::shared_ptr<FilterRecipe>> filters;
    };

    class SurfaceFilterRecipe : public FilterRecipe {
      public:
        SurfaceFilterRecipe() {}
        virtual std::string type() { return name; }
        virtual std::shared_ptr<CellSelector> getCellSelector() {
            return std::make_shared<DimensionalitySelector>(2);
        }

      private:
        const std::string name = "surface-filter";
    };

    class VolumeFilterRecipe : public FilterRecipe {
      public:
        VolumeFilterRecipe() {}
        virtual std::string type() { return name; }
        virtual std::shared_ptr<CellSelector> getCellSelector() {
            return std::make_shared<DimensionalitySelector>(3);
        }

      private:
        const std::string name = "volume-filter";
    };

    class TagFilterRecipe : public FilterRecipe {
      public:
        TagFilterRecipe(std::string s, const std::vector<int>& tags)
            : name(s), tags(tags.begin(), tags.end()) {}
        virtual std::string type() { return name; }
        virtual std::shared_ptr<CellSelector> getCellSelector() {
            std::vector<int> v(tags.begin(), tags.end());
            return std::make_shared<TagSelector>(std::move(v));
        }
        std::set<int> getTags() { return tags; }

      private:
        const std::string name;
        std::set<int> tags;
    };

    class SliceFilterRecipe : public FilterRecipe {
      public:
        SliceFilterRecipe(std::string s, double location) : name(s), location(location) {}

        std::string type() { return name; }
        std::shared_ptr<CellSelector> getCellSelector() { return nullptr; }
        double getLocation() { return location; }

      private:
        const std::string name;
        const double location;
    };

    typedef const std::string& Token;
    typedef std::queue<std::string> Queue;

    static Parfait::Lexer getLexer() {
        using namespace Parfait;
        Lexer::TokenMap token_map;
        token_map.push_back({Lexer::Matchers::whitespace(), ""});
        token_map.push_back({"save", "S"});
        token_map.push_back({"file", ""});
        token_map.push_back({"as", ""});
        token_map.push_back({"set", "SET"});
        token_map.push_back({"plugin", "PL"});
        token_map.push_back({"select", "SEL"});
        token_map.push_back({"fields", "FIELD"});
        token_map.push_back({"field", "FIELD"});
        token_map.push_back({"surfaces", "SURF"});
        token_map.push_back({"volume", "VOL"});
        token_map.push_back({Lexer::Matchers::comment("#"), ""});
        token_map.push_back({Lexer::Matchers::filename(), "R"});
        token_map.push_back({Lexer::Matchers::dashed_word(), "R"});
        token_map.push_back({Lexer::Matchers::c_variable(), "R"});
        Parfait::Lexer lexer(token_map);
        return lexer;
    }

    static bool isSaveCommand(Token t) { return t == "S"; }

    static void extractSaveCommand(std::vector<Recipe>& recipes, Queue& tokens) {
        tokens.pop();
        recipes.back().filename = tokens.front();
        tokens.pop();
        recipes.push_back(Recipe());
    }

    static std::queue<std::string> enqueue(const std::vector<std::string>& tokens) {
        Queue q;
        for (auto& t : tokens) q.push(t);
        return q;
    }

    static bool isSetCommand(Token t) { return t == "SET"; }

    static void extractSetCommand(std::vector<Recipe>& recipes, std::queue<std::string>& tokens) {
        tokens.pop();
        if (tokens.front() != "PL") throwCommandUnRecognized(tokens.front());
        tokens.pop();
        recipes.back().plugin = tokens.front();
        tokens.pop();
    }

    static bool isSelection(Token t) { return t == "SEL"; }

    static bool isKeyword(Token t) { return isSaveCommand(t) or isSetCommand(t) or isSelection(t); }

    static void extractFieldList(std::vector<Recipe>& recipes, Queue& tokens) {
        tokens.pop();
        while (not tokens.empty() and not isKeyword(tokens.front())) {
            printf("Adding field: %s\n", tokens.front().c_str());
            recipes.back().fields.push_back(tokens.front());
            tokens.pop();
        }
        printf("done adding fields\n");
    }

    static void extractSurfaceFilter(std::vector<Recipe>& recipes, Queue& tokens) {
        tokens.pop();
        recipes.back().filters.push_back(std::make_shared<SurfaceFilterRecipe>());
    }

    static void extractVolumeFilter(std::vector<Recipe>& recipes, Queue& tokens) {
        tokens.pop();
        recipes.back().filters.push_back(std::make_shared<VolumeFilterRecipe>());
    }

    static void extractSelectCommand(std::vector<Recipe>& recipes, Queue& tokens) {
        tokens.pop();
        auto t = tokens.front();
        if (t == "FIELD")
            extractFieldList(recipes, tokens);
        else if (t == "SURF")
            extractSurfaceFilter(recipes, tokens);
        else if (t == "VOL")
            extractVolumeFilter(recipes, tokens);
        else
            throw;
    }

    static std::vector<Recipe> parse(const std::string& s) {
        std::vector<Recipe> recipes(1);
        auto lexer = getLexer();
        auto tokens = enqueue(lexer.extractStrings(lexer.tokenize(s)));
        while (not tokens.empty()) {
            auto next = tokens.front();
            if (isSaveCommand(next))
                extractSaveCommand(recipes, tokens);
            else if (isSetCommand(next))
                extractSetCommand(recipes, tokens);
            else if (isSelection(next))
                extractSelectCommand(recipes, tokens);
            else
                throwCommandUnRecognized(tokens.front());
        }
        recipes.pop_back();

        return recipes;
    }

    static void throwCommandUnRecognized(const std::string& token) {
        throw std::logic_error("Command not recognized: " + token);
    }
};

}
