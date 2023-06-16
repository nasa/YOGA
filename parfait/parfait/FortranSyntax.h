#pragma once
#include <parfait/Lexer.h>

namespace Parfait {

class FortranSyntax {
  public:
    static Lexer::TokenMap syntax() {
        Lexer::TokenMap tm;
        tm.push_back({Lexer::Matchers::newline(), newline()});
        tm.push_back({Lexer::Matchers::whitespace(), ""});
        tm.push_back({"end", end()});
        tm.push_back({"begin", begin()});
        tm.push_back({"program", program()});
        tm.push_back({"module", module()});
        tm.push_back({"logical", logical()});
        tm.push_back({"if", If()});
        tm.push_back({",", comma()});
        tm.push_back({"::", doubleColon()});
        tm.push_back({":", colon()});
        tm.push_back({";", semicolon()});
        tm.push_back({"_", underscore()});
        tm.push_back({"//", concatination()});
        tm.push_back({"=>", associate()});
        tm.push_back({"/=", notEquals()});
        tm.push_back({"==", doubleEquals()});
        tm.push_back({"=", equals()});
        tm.push_back({">", greater()});
        tm.push_back({"<", less()});
        tm.push_back({"*", star()});
        tm.push_back({"/", divide()});
        tm.push_back({"+", plus()});
        tm.push_back({"-", minus()});
        tm.push_back({"%", percent()});
        tm.push_back({"(", openParentheses()});
        tm.push_back({")", closeParentheses()});
        tm.push_back({".true.", dottrue()});
        tm.push_back({".false.", dotfalse()});
        tm.push_back({".not.", dotnot()});
        tm.push_back({".or.", dotor()});
        tm.push_back({"or.", dotor()});
        tm.push_back({".and.", dotand()});
        tm.push_back({"and.", dotand()});
        tm.push_back({"&", lineContinuation()});
        tm.push_back({Lexer::Matchers::c_variable(), "#R"});
        tm.push_back({Lexer::Matchers::integer(), "IR"});
        tm.push_back({Lexer::Matchers::number(), "FR"});
        tm.push_back({Lexer::Matchers::double_quoted(), "SR"});
        tm.push_back({Lexer::Matchers::single_quoted(), "SR"});
        tm.push_back({Lexer::Matchers::comment("!"), ""});
        return tm;
    }

    static std::string begin() { return "B"; }
    static std::string end() { return "E"; }
    static std::string newline() { return "NL"; }
    static std::string program() { return "PROG"; }
    static std::string module() { return "MOD"; }
    static std::string logical() { return "LOG"; }
    static std::string If() { return "IF"; }
    static std::string comma() { return "COM"; }
    static std::string doubleColon() { return "DCOL"; }
    static std::string colon() { return "COL"; }
    static std::string associate() { return "ASS"; }
    static std::string star() { return "STAR"; }
    static std::string plus() { return "PLUS"; }
    static std::string minus() { return "MINUS"; }
    static std::string divide() { return "DIV"; }
    static std::string percent() { return "PCNT"; }
    static std::string semicolon() { return "SCOL"; }
    static std::string underscore() { return "USCORE"; }
    static std::string concatination() { return "CONCAT"; }
    static std::string openParentheses() { return "OPAREN"; }
    static std::string closeParentheses() { return "CPAREN"; }
    static std::string greater() { return "GT"; }
    static std::string less() { return "LT"; }
    static std::string equals() { return "EQ"; }
    static std::string doubleEquals() { return "DEQ"; }
    static std::string notEquals() { return "NEQ"; }
    static std::string dottrue() { return "TRUE"; }
    static std::string dotfalse() { return "FALSE"; }
    static std::string dotnot() { return "NOT"; }
    static std::string dotor() { return "OR"; }
    static std::string dotand() { return "AND"; }
    static std::string lineContinuation() { return "LC"; }
};
}
