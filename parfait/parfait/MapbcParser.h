#pragma once
#include <map>
#include <queue>
#include <parfait/Lexer.h>

namespace Parfait {

typedef std::map<int, std::pair<int, std::string>> BoundaryConditionMap;
class MapbcParser {
  public:
    static BoundaryConditionMap parse(const std::string& s) {
        Lexer lexer(mapbcSyntax());
        auto tokens = enqueue(lexer.tokenize(s));

        BoundaryConditionMap bc_map;

        int ntags = extractInt(tokens);
        ignoreUntilNewline(tokens);
        for (int i = 0; i < ntags; i++) {
            int tag = extractInt(tokens);
            int bc_number = extractInt(tokens);
            auto bc_name = extractName(tokens);
            bc_map[tag] = {bc_number, bc_name};
        }

        return bc_map;
    }

  private:
    inline static Lexer::TokenMap mapbcSyntax() {
        Lexer::TokenMap token_map;
        token_map.push_back({Lexer::Matchers::newline(), "NL"});
        token_map.push_back({Lexer::Matchers::whitespace(), ""});
        token_map.push_back({Lexer::Matchers::integer(), "IR"});
        token_map.push_back({Lexer::Matchers::dashed_word(), "SR"});
        token_map.push_back({Lexer::Matchers::c_variable(), "SR"});
        token_map.push_back({"/", ""});
        return token_map;
    }
    typedef std::queue<Lexer::Token> Queue;

    static void ignoreUntilNewline(Queue& q) {
        while (not q.empty()) {
            auto front = q.front().to_string();
            q.pop();
            if ("NL" == front) break;
        }
    }

    static std::string extractName(Queue& q) {
        std::string name = q.front().to_string();
        name = name.substr(1, name.npos);
        q.pop();
        while (not q.empty()) {
            auto s = q.front().to_string();
            q.pop();
            if (s == "NL") {
                break;
            } else {
                name += s.substr(1, s.npos);
            }
        }
        return name;
    }

    static int extractInt(Queue& q) {
        expectInt(q.front());
        auto s = q.front().to_string();
        q.pop();
        s = s.substr(1, s.npos);
        return StringTools::toInt(s);
    }
    static void expectInt(const Lexer::Token& token) {
        if (not isInt(token)) throw std::logic_error("MapbcParser expected Integer, but found: " + token.to_string());
    }

    static bool isInt(const Lexer::Token& token) { return token.to_string().front() == 'I'; }

    inline static std::queue<Lexer::Token> enqueue(const std::vector<Lexer::Token>& tokens) {
        Queue q;
        for (auto& token : tokens) q.push(token);
        return q;
    }
};
}