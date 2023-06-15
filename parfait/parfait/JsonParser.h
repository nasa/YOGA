#pragma once
#include <string>
#include <limits>
#include <cmath>
#include <queue>
#include "Dictionary.h"
#include "Lexer.h"

namespace Parfait {
class JsonParser {
  public:
    inline static Dictionary parse(const std::string& s) {
        if (s.empty()) return {};
        Lexer lexer(jsonSyntax());
        auto tokens = enqueue(lexer.tokenize(s));
        std::string first_token = tokens.front().to_string();
        if (first_token == beginObject())
            return extractObject(tokens);
        else if (first_token == beginArray())
            return extractArray(tokens);
        else
            throw ParseException("JSON string must be an object or array.  First token: " + first_token);
    }

    inline static Lexer::TokenMap jsonSyntax() {
        Lexer::TokenMap token_map;
        token_map.push_back({Lexer::Matchers::whitespace(), ""});
        token_map.push_back({"{", beginObject()});
        token_map.push_back({"}", endObject()});
        token_map.push_back({"[", beginArray()});
        token_map.push_back({"]", endArray()});
        token_map.push_back({":", colon()});
        token_map.push_back({",", comma()});
        token_map.push_back({"Rtrue", truth()});
        token_map.push_back({"Rfalse", untruth()});
        token_map.push_back({"Rnull", null()});
        token_map.push_back({Lexer::Matchers::double_quoted(), "#R#\"="});
        token_map.push_back({Lexer::Matchers::integer(), "IR"});
        token_map.push_back({Lexer::Matchers::number(), "FR"});
        return token_map;
    }

  private:
    typedef std::queue<Lexer::Token> Queue;
    inline static std::string colon() { return "COL"; }
    inline static std::string comma() { return "COM"; }
    inline static std::string truth() { return "T"; }
    inline static std::string null() { return "NUL"; }
    inline static std::string untruth() { return "U"; }
    inline static std::string beginObject() { return "BO"; }
    inline static std::string endObject() { return "EO"; }
    inline static std::string beginArray() { return "BA"; }
    inline static std::string endArray() { return "EA"; }

    inline static std::queue<Lexer::Token> enqueue(const std::vector<Lexer::Token>& tokens) {
        Queue q;
        for (auto& token : tokens) q.push(token);
        return q;
    }

    inline static Dictionary extractObject(Queue& q) {
        consume(beginObject(), q);
        auto json = extractItems(q);
        consume(endObject(), q);
        return json;
    }

    inline static Dictionary extractItems(Queue& q) {
        throwIfEmpty(q);
        Dictionary json;
        if (endObject() == q.front().to_string()) return json;
        addKeyValuePair(json, q);
        while (not q.empty() and comma() == q.front().to_string()) {
            q.pop();
            addKeyValuePair(json, q);
        }
        return json;
    }

    inline static void addKeyValuePair(Dictionary& json, Queue& q) {
        auto key = convertLiteral(q.front().to_string());
        q.pop();
        consume(colon(), q);
        json[key] = extractValue(q);
    }

    inline static Dictionary extractValue(Queue& q) {
        auto next = q.front().to_string();
        if (isNumber(next))
            return extractNumber(q);
        else if (isBoolean(next))
            return extractBoolean(q);
        else if (isNull(next))
            return extractNull(q);
        else if (isLiteral(next))
            return extractString(q);
        else if (beginObject() == next)
            return extractObject(q);
        else if (beginArray() == next)
            return extractArray(q);
        else
            throw ParseException("Unexpected token: " + next);
        return Dictionary();
    }

    inline static bool isInteger(const std::string& token) { return 'I' == token.front(); }
    inline static bool isFloatingPoint(const std::string& token) { return 'F' == token.front(); }
    inline static bool isNumber(const std::string& token) { return isInteger(token) or isFloatingPoint(token); }
    inline static bool isBoolean(const std::string& token) { return truth() == token or untruth() == token; }
    inline static bool isNull(const std::string& token) { return null() == token; }
    inline static bool isLiteral(const std::string& token) { return '#' == token.front() and '#' == token.back(); }

    inline static Dictionary extractFloatingPoint(Queue& q) {
        Dictionary value;
        auto next = popToken(q).to_string();
        value = StringTools::toDouble(next.substr(1, next.npos));
        return value;
    }

    inline static bool numberIsInt(double x) {
        double intpart;
        return std::modf(x, &intpart) == 0.0 and x < std::numeric_limits<int>::max();
    }

    inline static Dictionary extractNumber(Queue& q) {
        bool parsed_as_int = isInteger(q.front().to_string());
        auto next = popToken(q).to_string();
        double x = StringTools::toDouble(next.substr(1, next.npos));
        Dictionary value;
        if (parsed_as_int and numberIsInt(x)) {
            value = static_cast<int>(x);
        } else {
            value = x;
        }
        return value;
    }

    inline static std::vector<bool> extractBooleans(Queue& q) {
        std::vector<bool> v;
        v.push_back(extractBoolean(q).asBool());
        while (comma() == q.front().to_string()) {
            q.pop();
            v.push_back(extractBoolean(q).asBool());
        }
        return v;
    }

    inline static Dictionary extractBoolean(Queue& q) {
        Dictionary value;
        auto next = popToken(q).to_string();
        value = truth() == next;
        return value;
    }

    inline static Dictionary extractNull(Queue& q) {
        Dictionary value;
        auto next = popToken(q).to_string();
        value = Dictionary::null();
        return value;
    }

    inline static Dictionary extractArray(Queue& q) {
        consume(beginArray(), q);
        Dictionary value;
        auto next = q.front().to_string();
        if (endArray() == next)
            value = std::vector<int>();
        else if (isNumber(next))
            value = extractNumbers(q);
        else if (isLiteral(next))
            value = extractStrings(q);
        else if (isBoolean(next))
            value = extractBooleans(q);
        else if (beginObject() == next)
            value = extractObjects(q);
        else if (beginArray() == next)
            value = extractArrays(q);
        consume(endArray(), q);
        return value;
    }

    inline static Dictionary extractNumbers(Queue& q) {
        std::vector<double> v;
        v.push_back(extractFloatingPoint(q).asDouble());
        while (comma() == q.front().to_string()) {
            q.pop();
            v.push_back(extractFloatingPoint(q).asDouble());
        }

        // FIXME: Dictionary needs constructors that match assignment operators
        Dictionary json;
        // If all numbers are integers, return
        for (const auto& x : v) {
            if (not numberIsInt(x)) {
                json = v;
                return json;
            };
        }

        std::vector<int> v_int(v.begin(), v.end());
        json = v;
        return json;
    }

    inline static Dictionary extractObjects(Queue& q) {
        Dictionary json;
        int index = 0;
        json[index++] = extractObject(q);
        while (comma() == q.front().to_string()) {
            consume(comma(), q);
            json[index++] = extractObject(q);
        }
        return json;
    }

    inline static Dictionary extractArrays(Queue& q) {
        Dictionary json;
        int index = 0;
        json[index++] = extractArray(q);
        while (comma() == q.front().to_string()) {
            consume(comma(), q);
            json[index++] = extractArray(q);
        }
        return json;
    }

    inline static std::vector<std::string> extractStrings(Queue& q) {
        std::vector<std::string> v;
        v.push_back(extractString(q).asString());
        while (comma() == q.front().to_string()) {
            q.pop();
            v.push_back(extractString(q).asString());
        }
        return v;
    }

    inline static Dictionary extractString(Queue& q) {
        Dictionary value;
        auto next = popToken(q).to_string();
        value = convertLiteral(next);
        return value;
    }

    inline static std::string convertLiteral(const std::string& s) { return s.substr(1, s.length() - 2); }

    inline static Lexer::Token popToken(Queue& q) {
        throwIfEmpty(q);
        auto s = q.front();
        q.pop();
        return s;
    }

    inline static void throwIfEmpty(const Queue& q) {
        if (q.empty()) throw ParseException("JsonParser: string ended unexpectedly");
    }

    inline static void consume(const std::string& expected, Queue& q) {
        auto actual = popToken(q).to_string();
        if (expected != actual) throw ParseException("JsonParser: expected: " + expected + " actual: " + actual);
    }
};
inline Dictionary parse(const std::string& s) { return JsonParser::parse(s); }
}

namespace Parfait {
namespace json_literals {
    inline Parfait::Dictionary operator""_json(const char* s, std::size_t n) {
        Parfait::Dictionary d = Parfait::JsonParser::parse(s);
        return d;
    }
}
}