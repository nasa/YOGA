#pragma once
#include <queue>
#include <parfait/Dictionary.h>
#include <parfait/Lexer.h>

namespace Parfait {

class PomlParser {
  public:
    inline static Parfait::Dictionary parse(const std::string& s) {
        Lexer lexer(pomlSyntax());
        auto tokens = enqueue(lexer.tokenize(s));
        Dictionary dictionary;
        while (not tokens.empty()) {
            auto token = tokens.front().to_string();
            if (isKey(token)) {
                extractKeyValuePair(dictionary, tokens);
            }
        }

        return dictionary;
    }

  private:
    typedef std::queue<Lexer::Token> Queue;
    inline static Lexer::TokenMap pomlSyntax() {
        Lexer::TokenMap token_map;
        token_map.push_back({Lexer::Matchers::comment("#"), ""});
        token_map.push_back({"true", "B:T"});
        token_map.push_back({"false", "B:F"});
        token_map.push_back({Lexer::Matchers::c_variable(), "Key:R"});
        token_map.push_back({Lexer::Matchers::double_quoted(), "String:R"});
        token_map.push_back({"=", "EQ"});
        token_map.push_back({"[", "BA"});
        token_map.push_back({"]", "EA"});
        token_map.push_back({Lexer::Matchers::integer(), "I:R"});
        token_map.push_back({Lexer::Matchers::number(), "F:R"});
        token_map.push_back({".", "DOT"});
        token_map.push_back({Lexer::Matchers::whitespace(), ""});
        return token_map;
    }

    inline static bool startsWith(const std::string& s, const std::string& prefix) {
        auto x = s.substr(0, prefix.length());
        return x == prefix;
    }

    inline static bool isKey(const std::string& s) {
        if (isQuotedString(s)) return true;
        if (s.length() < 5) return false;
        return startsWith(s, "Key:");
    }

    inline static bool isQuotedString(const std::string& s) {
        if (s.length() < 9) return false;
        return s[7] == '\"' and s.back() == '\"';
    }

    inline static bool isInteger(const std::string& s) { return s.front() == 'I'; }

    inline static std::string extractKey(Queue& q) {
        auto s = q.front().to_string();
        if (isQuotedString(s)) {
            return extractQuoted(q).asString();
        } else {
            q.pop();
            return s.substr(4, std::string::npos);
        }
    }

    inline static void consume(const std::string& expected, Queue& q) {
        if (expected == q.front().to_string())
            q.pop();
        else
            throw ParseException("Expected: " + expected + " found: " + q.front().to_string());
    }

    inline static Dictionary extractQuoted(Queue& q) {
        auto s = q.front().to_string();
        q.pop();
        Dictionary entry;
        s = s.substr(8, s.npos);
        s.pop_back();
        entry = s;
        return entry;
    }

    inline static Dictionary extractInteger(Queue& q) {
        auto s = q.front().to_string();
        q.pop();
        s = s.substr(2, s.npos);
        Dictionary entry;
        entry = StringTools::toInt(s);
        return entry;
    }

    inline static bool isBool(const std::string& s) { return startsWith(s, "B:"); }

    inline static bool isArray(const std::string& s) { return s == "BA"; }

    inline static Dictionary extractBool(Queue& q) {
        auto s = q.front().to_string();
        q.pop();
        bool x = s.back() == 'T';
        Dictionary entry;
        entry = x;
        return entry;
    }

    inline static bool isFloatingPoint(const std::string& s) { return s.front() == 'F'; }
    inline static std::vector<bool> extractBools(Queue& q) {
        std::vector<bool> v;
        v.push_back(extractBool(q).asBool());
        while ("," == q.front().to_string()) {
            q.pop();
            v.push_back(extractBool(q).asBool());
        }
        return v;
    }
    inline static std::vector<double> extractFloats(Queue& q) {
        std::vector<double> v;
        v.push_back(extractFloatingPoint(q).asDouble());
        while ("," == q.front().to_string()) {
            q.pop();
            v.push_back(extractFloatingPoint(q).asDouble());
        }
        return v;
    }
    inline static std::vector<int> extractInts(Queue& q) {
        std::vector<int> v;
        v.push_back(extractInteger(q).asInt());
        while ("," == q.front().to_string()) {
            q.pop();
            v.push_back(extractInteger(q).asInt());
        }
        return v;
    }
    inline static std::vector<std::string> extractStrings(Queue& q) {
        std::vector<std::string> v;
        v.push_back(extractQuoted(q).asString());
        while ("," == q.front().to_string()) {
            q.pop();
            v.push_back(extractQuoted(q).asString());
        }
        return v;
    }
    inline static Dictionary extractArray(Queue& q) {
        consume("BA", q);
        Dictionary value;
        auto next = q.front().to_string();
        if ("EA" == next)
            value = std::vector<int>();
        else if (isFloatingPoint(next))
            value = extractFloats(q);
        else if (isInteger(next))
            value = extractInts(q);
        else if (isQuotedString(next))
            value = extractStrings(q);
        else if (isBool(next))
            value = extractBools(q);
        consume("EA", q);
        return value;
    }

    inline static Dictionary extractFloatingPoint(Queue& q) {
        auto s = q.front().to_string();
        q.pop();
        s = s.substr(2, s.npos);
        double x = StringTools::toDouble(s);
        Dictionary entry;
        entry = x;
        return entry;
    }

    inline static Dictionary extractValue(Queue& q) {
        auto s = q.front().to_string();
        if (isQuotedString(s))
            return extractQuoted(q);
        else if (isInteger(s))
            return extractInteger(q);
        else if (isFloatingPoint(s))
            return extractFloatingPoint(q);
        else if (isBool(s))
            return extractBool(q);
        else if (isArray(s))
            return extractArray(q);
        else
            throw ParseException("Don't know how to handle value: " + s);

        Dictionary empty;
        return empty;
    }

    inline static bool isDot(const std::string& s) { return "DOT" == s; }

    inline static void extractKeyValuePair(Dictionary& dictionary, Queue& q) {
        std::string key = extractKey(q);
        Dictionary* entry = &dictionary;
        while (isDot(q.front().to_string())) {
            consume("DOT", q);
            entry = &(*entry)[key];
            key = extractKey(q);
        }
        consume("EQ", q);
        (*entry)[key] = extractValue(q);
    }

    inline static std::queue<Lexer::Token> enqueue(const std::vector<Lexer::Token>& tokens) {
        Queue q;
        for (auto& token : tokens) q.push(token);
        return q;
    }
};
}
