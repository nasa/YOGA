#pragma once
#include <queue>
#include "Dictionary.h"
#include "Lexer.h"
#include <parfait/StringTools.h>

#include <iostream>
#include <parfait/Checkpoint.h>
namespace Parfait {
class NamelistParser {
  public:
    inline static Dictionary parse(const std::string& s) {
        if (s.empty()) return {};
        Lexer lexer(namelistSyntax());
        auto tokens = enqueue(lexer.tokenize(s));
        return extractNamelists(tokens);
    }
    inline static Lexer::TokenMap namelistSyntax() {
        Lexer::TokenMap token_map;
        // token_map.push_back({Lexer::Matchers::newline(), ""});
        token_map.push_back({Lexer::Matchers::whitespace(), ""});
        token_map.push_back({",", ""});
        token_map.push_back({"&", beginNamelist()});
        token_map.push_back({"$", beginEndNamelistOld()});
        token_map.push_back({"/", endNamelist()});
        token_map.push_back({"=", assignment()});
        token_map.push_back({"*", multiply()});
        token_map.push_back({"R\\.[tT]\\.", truth()});
        token_map.push_back({"R\\.[fF]\\.", untruth()});
        token_map.push_back({"R\\.[tT][rR][uU][eE]\\.", truth()});
        token_map.push_back({"R\\.[fF][aA][lL][sS][eE]\\.", untruth()});
        token_map.push_back({"R\\(.*\\)", "PR"});
        token_map.push_back({Lexer::Matchers::c_variable(), "#R"});
        token_map.push_back({Lexer::Matchers::integer(), "IR"});
        token_map.push_back({Lexer::Matchers::number(), "FR"});
        token_map.push_back({Lexer::Matchers::double_quoted(), "SR"});
        token_map.push_back({Lexer::Matchers::single_quoted(), "SR"});
        token_map.push_back({Lexer::Matchers::comment("!"), ""});
        return token_map;
    }

  private:
    typedef std::queue<Lexer::Token> Queue;
    typedef std::vector<Lexer::Token> Vector;
    enum ValueType { Unknown, Number, Bool, String };
    inline static std::string valueTypeName(ValueType v) {
        switch (v) {
            case (ValueType::Unknown):
                return "Unknown";
                break;
            case (ValueType::Number):
                return "Number";
                break;
            case (ValueType::Bool):
                return "Bool";
                break;
            case (ValueType::String):
                return "String";
                break;
            default:
                return "Invalid Type";
        }
    }

    inline static std::string beginNamelist() { return "BN"; }
    inline static std::string endNamelist() { return "EN"; }
    inline static std::string beginEndNamelistOld() { return "BENO"; }
    inline static std::string assignment() { return "EQ"; }
    inline static std::string multiply() { return "MULT"; }
    inline static std::string truth() { return "T"; }
    inline static std::string untruth() { return "U"; }

    inline static std::queue<Lexer::Token> enqueue(const Vector& tokens) {
        Queue q;
        for (size_t i = 0; i < tokens.size() - 1; i++) {
            if (isParenGroup(tokens[i].to_string())) {
                throw ParseException("Parser does not yet support setting portions of arrays! token: " +
                                     tokens[i].to_string());
            } else if (assignment() == lookAhead(i + 2, tokens) and isEmptyParenGroup(lookAhead(i + 1, tokens))) {
                q.push(Lexer::Token(tokens[i].lineNumber(), tokens[i].column(), "AS#" + tokens[i].to_string()));
                i += 2;
            } else if (assignment() == lookAhead(i + 1, tokens)) {
                q.push(Lexer::Token(tokens[i].lineNumber(), tokens[i].column(), "AS#" + tokens[i].to_string()));
                i++;
            } else {
                q.push(tokens[i]);
            }
        }
        if (not isNamelistEnd(tokens.back().to_string())) {
            throw ParseException("Something went wrong! Last token is not end of namelist! " +
                                 tokens.back().to_string());
        } else {
            q.push(tokens.back());
        }
        return q;
    }
    static std::string lookAhead(size_t i, const Vector& tokens) {
        if (i >= tokens.size()) return "";
        return tokens[i].to_string();
    }

    inline static Dictionary extractNamelists(Queue& q) {
        Dictionary json;
        int index = 0;
        while (not q.empty()) {
            if (q.front().to_string() == beginNamelist())
                json[index++] = extractNamelist(q);
            else if (q.front().to_string() == beginEndNamelistOld())
                json[index++] = extractOldNamelist(q);
            else
                throw ParseException("Expecting start of namelist!" + q.front().to_string());
        }
        return json;
    }

    inline static Dictionary extractNamelist(Queue& q) {
        Dictionary json;
        consume(beginNamelist(), q);
        throwIfEmpty(q);
        auto key = extractVariableName(q.front().to_string());
        q.pop();
        json[key] = extractItems(q);
        consume(endNamelist(), q);
        return json;
    }

    inline static Dictionary extractOldNamelist(Queue& q) {
        Dictionary json;
        consume(beginEndNamelistOld(), q);
        throwIfEmpty(q);
        auto key = extractVariableName(q.front().to_string());
        q.pop();
        json[key] = extractItems(q);
        consume(beginEndNamelistOld(), q);
        return json;
    }

    inline static Dictionary extractItems(Queue& q) {
        Dictionary json;
        while (not isNamelistEnd(q.front().to_string())) {
            extractItem(json, q);
        }
        return json;
    }

    inline static void extractItem(Dictionary& json, Queue& q) {
        auto key = extractVariableName(q.front().to_string());
        q.pop();
        json[key] = extractValue(q);
    }

    inline static Dictionary extractValue(Queue& q) {
        Vector value_vec;
        while ((not q.empty()) and (isValue(q.front().to_string()) or isMult(q.front().to_string()))) {
            value_vec.push_back(popToken(q));
        }

        if (value_vec.empty()) {
            auto& token = q.front();
            auto line = std::to_string(token.lineNumber());
            auto col = std::to_string(token.column());
            throw ParseException("Unexpected token: " + token.to_string() + "\nLine: " + line + " column: " + col +
                                 "\nNote: undeliminated strings are not supported!" +
                                 "\nNote: only strict logicals (.true. .t. true t) supported");
        }

        auto front = value_vec.front().to_string();

        if (value_vec.size() != 1)
            return extractArray(value_vec);
        else if (isNumber(front))
            return extractNumber(value_vec[0]);
        else if (isString(front))
            return extractString(value_vec[0]);
        else if (isBool(front))
            return extractBool(value_vec[0]);
        else
            throw ParseException("Unexpected token: " + q.front().to_string());
        return Dictionary();
    }

    inline static bool isMult(const std::string& token) { return multiply() == token; }
    inline static bool isInteger(const std::string& token) { return 'I' == token.front(); }
    inline static bool isFloatingPoint(const std::string& token) { return 'F' == token.front(); }
    inline static bool isNumber(const std::string& token) { return isInteger(token) or isFloatingPoint(token); }
    inline static bool isBool(const std::string& token) { return truth() == token or untruth() == token; }
    inline static bool isParenGroup(const std::string& token) { return 'P' == token.front(); }
    inline static bool isEmptyParenGroup(const std::string& token) { return "P(:)" == token; }
    inline static bool isString(const std::string& token) { return 'S' == token.front(); }
    inline static bool isCVariable(const std::string& token) { return '#' == token.front(); }
    inline static bool isValue(const std::string& token) {
        return isNumber(token) or isBool(token) or isString(token) or isCVariable(token);
    }
    inline static bool isAssignmentVariable(const std::string& token) { return token.rfind("AS##") == 0; }
    inline static bool isNamelistStart(const std::string& token) {
        return token == beginNamelist() or token == beginEndNamelistOld();
    }
    inline static bool isNamelistEnd(const std::string& token) {
        return token == endNamelist() or token == beginEndNamelistOld();
    }
    inline static bool numberIsInt(double x) {
        double intpart;
        return std::modf(x, &intpart) == 0.0 and x < std::numeric_limits<int>::max();
    }

    inline static Dictionary extractNumber(const Lexer::Token& t) {
        auto token = t.to_string();
        double x = StringTools::toDouble(trimTag(token, 1));
        Dictionary value;
        if (isInteger(token) and numberIsInt(x))
            value = static_cast<int>(x);
        else
            value = x;
        return value;
    }

    inline static Dictionary extractString(const Lexer::Token& t) {
        Dictionary value;
        auto token = t.to_string();
        value = token.substr(2, token.length() - 3);
        return value;
    }

    inline static Dictionary extractBool(const Lexer::Token& t) {
        Dictionary value;
        auto token = t.to_string();
        value = token == truth();
        return value;
    }

    inline static Dictionary extractArray(const Vector& v) {
        auto type = determineArrayType(v);
        switch (type) {
            case ValueType::Number:
                return extractNumericArray(v);
                break;
            case ValueType::Bool:
                return extractBoolArray(v);
                break;
            default:  // Unknown
                throw ParseException("Cannot parse an array of type: " + valueTypeName(type));
                return Dictionary();
        }
    }

    inline static ValueType determineArrayType(const Vector& v) {
        ValueType arrayType = ValueType::Unknown;
        for (size_t i = 0; i < v.size(); i++) {
            auto next = v[i];
            if (i != v.size() - 1 and isMult(v[i + 1].to_string())) {
                if (isInteger(v[i].to_string())) {
                    i = i + 2;
                    auto value = v[i];  // pop value
                    updateArrayType(arrayType, determineValueType(value));
                } else {
                    throw ParseException("Cannot use multiply notation with float: " + next.to_string());
                }
            } else {
                updateArrayType(arrayType, determineValueType(next));
            }
        }

        return arrayType;
    }

    inline static ValueType determineValueType(const Lexer::Token& t) {
        auto t_string = t.to_string();
        if (isNumber(t_string)) {
            return ValueType::Number;
        } else if (isString(t_string)) {
            return ValueType::String;
        } else if (isBool(t_string)) {
            return ValueType::Bool;
        } else {
            throw ParseException("Expected a valid value type: " + t.to_string());
            return ValueType::Unknown;
        }
    }

    inline static void updateArrayType(ValueType& arrayType, const ValueType& valueType) {
        if (arrayType == ValueType::Unknown) {
            arrayType = valueType;
        } else if (valueType != arrayType) {
            throw ParseException("Cannot mix different value types in arrays!");
        }
    }

    inline static Dictionary extractNumericArray(const Vector& v) {
        std::vector<double> vector;
        for (size_t i = 0; i < v.size(); i++) {
            auto next = v[i].to_string();
            if (i != v.size() - 1 and isMult(v[i + 1].to_string())) {
                if (isInteger(next)) {
                    const int repeat = StringTools::toInt(trimTag(next, 1));
                    i = i + 2;
                    auto value_string = v[i].to_string();  // pop value
                    auto value = StringTools::toDouble(trimTag(value_string, 1));
                    for (int n = 0; n < repeat; n++) {
                        vector.push_back(value);
                    }
                } else {
                    throw ParseException("Cannot use multiply notation with float: " + next);
                }
            } else {
                vector.push_back(StringTools::toDouble(trimTag(next, 1)));
            }
        }
        Dictionary dict_array;
        dict_array = vector;
        return dict_array;
    }

    inline static Dictionary extractBoolArray(const Vector& v) {
        std::vector<bool> vector;
        for (size_t i = 0; i < v.size(); i++) {
            auto next = v[i].to_string();
            if (i != v.size() - 1 and isMult(v[i + 1].to_string())) {
                if (isInteger(next)) {
                    const int repeat = StringTools::toInt(trimTag(next, 1));
                    i = i + 2;
                    auto value_string = v[i].to_string();  // pop value
                    auto value = value_string == truth();
                    for (int n = 0; n < repeat; n++) {
                        vector.push_back(value);
                    }
                } else {
                    throw ParseException("Cannot use multiply notation with float: " + next);
                }
            } else {
                vector.push_back(next == truth());
            }
        }
        Dictionary dict_array;
        dict_array = vector;
        return dict_array;
    }

    inline static std::string extractVariableName(const std::string& s) {
        if (isCVariable(s))
            return StringTools::tolower(trimTag(s, 1));
        else if (isAssignmentVariable(s))
            return StringTools::tolower(trimTag(s, 4));
        else
            throw ParseException("Expected a variable name got: " + s);
        return "";
    }

    inline static std::string trimTag(const std::string& s, int tag_length) {
        return s.substr(tag_length, s.length() - tag_length);
    }

    inline static void throwIfEmpty(const Queue& q) {
        if (q.empty()) throw ParseException("NamelistParser: string ended unexpectedly");
    }

    inline static Lexer::Token popToken(Queue& q) {
        throwIfEmpty(q);
        auto s = q.front();
        q.pop();
        return s;
    }

    inline static void consume(const std::string& expected, Queue& q) {
        auto actual = popToken(q).to_string();
        if (expected != actual) throw ParseException("JsonParser: expected: " + expected + " actual: " + actual);
    }
};
}
