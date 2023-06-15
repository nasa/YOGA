#pragma once
#include <parfait/StringTools.h>
#include <parfait/FortranSyntax.h>

namespace YOGA {

class MovingBodyInputParser {
  public:
    struct Body {
        std::string name;
        std::vector<int> tags;
    };

    static std::vector<Body> parse(const std::string& input) {
        using namespace Parfait;
        Lexer lexer(FortranSyntax::syntax());
        auto tokens = lexer.tokenize(input);
        int n = extractMovingBodyCount(tokens);
        std::vector<Body> bodies(n);
        for (int i = 0; i < n; i++) {
            bodies[i].name = extractBodyName(tokens, i);
            bodies[i].tags = extractBoundaryTags(tokens, i);
        }
        return bodies;
    }

  private:
    static int extractMovingBodyCount(const std::vector<Parfait::Lexer::Token>& tokens) {
        for (size_t i = 0; i < tokens.size(); i++) {
            auto token = tokens[i].to_string();
            if ("#n_moving_bodies" == token) {
                token = tokens[i + 2].to_string();
                return Integer(token);
            }
        }
        return 0;
    }

    static std::string extractBodyName(const std::vector<Parfait::Lexer::Token>& tokens, int id) {
        int fortran = 1;
        id += fortran;
        for (size_t i = 0; i < tokens.size(); i++) {
            auto token = tokens[i].to_string();
            if ("#body_name" == token) {
                auto id_token = tokens[i + 2];
                int body_id = Integer(id_token.to_string());
                if (id == body_id) {
                    auto name_token = tokens[i + 5].to_string();
                    return name_token.substr(2, name_token.length() - 3);
                }
            }
        }
        printf("Could not find body(%i)\n", id);
        return "";
    }

    static std::vector<int> extractBoundaryTags(const std::vector<Parfait::Lexer::Token>& tokens, int id) {
        int fortran = 1;
        id += fortran;
        std::vector<int> tags;
        for (size_t i = 0; i < tokens.size(); i++) {
            auto token = tokens[i].to_string();
            if ("#defining_bndry" == token) {
                auto id_token = tokens[i + 4];
                int body_id = Integer(id_token.to_string());
                if (id == body_id) {
                    auto tag_token = tokens[i + 7].to_string();
                    tags.push_back(Parfait::StringTools::toInt(tag_token.substr(1, tag_token.npos)));
                }
            }
        }
        return tags;
    }

    static int Integer(const std::string& token) {
        auto s = token.substr(1, token.npos);
        return Parfait::StringTools::toInt(s);
    }
};
}