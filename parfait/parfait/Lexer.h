#pragma once
#include <parfait/StringTools.h>
#include <regex>
#include <string>
#include <vector>
#include <utility>
#include <parfait/RegexMatchers.h>

namespace Parfait {

class ParseException : public std::exception {
  public:
    inline ParseException(std::string msg) : msg(msg){};
    inline const char* what() const noexcept { return msg.c_str(); }

  private:
    std::string msg;
};

class Lexer {
  public:
    typedef std::pair<std::string, std::string> TokenMatcherPair;
    typedef std::vector<TokenMatcherPair> TokenMap;
    class Token {
      public:
        Token(int line_in, int column_in, const std::string s_in) : line(line_in), col(column_in), s(s_in) {}
        std::string to_string() const { return s; }
        int lineNumber() const { return line + 1; }
        int column() const { return col + 1; }

      private:
        int line;
        int col;
        std::string s;
    };

    class Matchers {
      public:
        static std::string filename() { return "R" + RegexMatchers::filename(); }
        static std::string c_variable() { return "R" + RegexMatchers::c_variable(); }
        static std::string dashed_word() { return "R" + RegexMatchers::dashed_word(); }
        static std::string alphabetic_word() { return "R" + RegexMatchers::alphabetic_word(); }
        static std::string whitespace() { return "R" + RegexMatchers::whitespace(); }
        static std::string double_quoted() { return "R" + RegexMatchers::double_quoted(); }
        static std::string single_quoted() { return "R" + RegexMatchers::single_quoted(); }
        static std::string square_bracketed() { return "R" + RegexMatchers::square_bracketed(); }
        static std::string integer() { return "R" + RegexMatchers::integer(); }
        static std::string number() { return "R" + RegexMatchers::number(); }
        static std::string comment(const std::string starts_with) { return "R" + starts_with + ".*"; }
        static std::string newline() { return "\n"; }
    };

    explicit Lexer(const TokenMap& m) : plain_map(buildPlainMatchMap(m)), regex_map(buildRegexMatchMap(m)) {}

    std::vector<Token> tokenize(const std::string& s) {
        if (s.empty()) return {};
        current_line_number = current_column = 0;
        auto lines = Parfait::StringTools::split(s, "\n", false);
        std::vector<Token> tokens;
        auto newline_token = getNewlineToken();
        for (auto& line : lines) {
            size_t pos = 0;
            while (pos < line.length()) insertNextToken(line, tokens, pos);
            if (not newline_token.empty()) tokens.emplace_back(current_line_number, current_column, newline_token);
            current_line_number++;
            current_column = 0;
        }
        if (not newline_token.empty() and not tokens.empty()) tokens.pop_back();
        return tokens;
    }

    static std::vector<std::string> extractStrings(const std::vector<Token>& tokens) {
        std::vector<std::string> strings;
        for (auto& token : tokens) strings.push_back(token.to_string());
        return strings;
    }

    std::string getNewlineToken() {
        for (auto& pair : plain_map)
            if (pair.first == Lexer::Matchers::newline()) return pair.second;
        return "";
    }

    void insertNextToken(const std::string& s, std::vector<Token>& tokens, size_t& pos) {
        for (const auto& pair : plain_map) {
            std::string result;
            size_t old_position = pos;
            if (plainMatch(pair, s, result, pos)) {
                if (not result.empty()) tokens.emplace_back(current_line_number, current_column, result);
                current_column += pos - old_position;
                return;
            }
        }
        for (const auto& pair : regex_map) {
            std::string result;
            size_t old_position = pos;
            if (regexMatch(pair, s, result, pos)) {
                if (not result.empty()) tokens.emplace_back(current_line_number, current_column, result);
                current_column += pos - old_position;
                return;
            }
        }
        std::string error_msg = "Lexer error (line ";
        error_msg += std::to_string(current_line_number);
        error_msg += ", column ";
        error_msg += std::to_string(current_column);
        error_msg += ")\n";
        error_msg +=
            "Could not find next token in: \"" + s.substr(pos, std::min(size_t(20), s.length() - pos)) + "\"...\n";
        throw ParseException(error_msg);
    }
    bool isNewline(const std::string& s, const size_t& pos) const { return matchesFromPosition(s, pos, "\n"); }

  private:
    typedef std::pair<std::regex, std::string> RegexPair;
    typedef std::vector<RegexPair> RegexMap;
    TokenMap plain_map;
    RegexMap regex_map;
    int current_line_number;
    int current_column;

    TokenMap buildPlainMatchMap(const TokenMap& map) const {
        TokenMap plain_matches_map;
        for (const auto& token_pair : map)
            if (not isRegularExpression(token_pair.first)) plain_matches_map.push_back(token_pair);
        return plain_matches_map;
    }

    RegexMap buildRegexMatchMap(const TokenMap& map) const {
        RegexMap regex_matches_map;
        for (const auto& token_pair : map) {
            if (isRegularExpression(token_pair.first)) {
                std::regex expression(token_pair.first.substr(1, token_pair.first.npos));
                regex_matches_map.push_back({expression, token_pair.second});
            }
        }
        return regex_matches_map;
    }

    bool matchesFromPosition(const std::string& s, size_t pos, const std::string& target) const {
        for (size_t i = 0; i < target.length(); i++) {
            if (s[pos + i] != target[i]) return false;
        }
        return true;
    }

    bool plainMatch(const TokenMatcherPair& token_pair, const std::string& s, std::string& result, size_t& pos) const {
        const auto& key = token_pair.first;
        if (matchesFromPosition(s, pos, key)) {
            result = token_pair.second;
            pos += key.length();
            return true;
        }
        return false;
    }

    bool isRegularExpression(const std::string& token) const { return token[0] == 'R'; }

    bool regexMatch(const RegexPair& token_pair, const std::string& s, std::string& result, size_t& pos) {
        auto tail = s.substr(pos, s.npos);
        std::cmatch m;
        bool found = std::regex_search(tail.c_str(), m, token_pair.first);
        if (found and m.position(0) == 0) {
            result = m[0].str();
            pos += result.length();
            result = formatResult(result, token_pair.second);
            return true;
        } else {
            return false;
        }
    }

    std::string formatResult(const std::string& s, std::string recipe) {
        using namespace Parfait::StringTools;
        std::string result = s;
        while (contains(recipe, "=")) {
            auto pos = recipe.find("=");
            char L = recipe[pos - 1];
            if (pos < recipe.length() - 1)
                result = findAndReplace(result, {L}, {recipe[pos + 1]});
            else
                result = findAndReplace(result, {L}, "");
            recipe = recipe.substr(0, pos - 1);
        }
        return findAndReplace(recipe, "R", result);
    }
};
}
