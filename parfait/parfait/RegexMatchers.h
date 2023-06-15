#pragma once
namespace Parfait {
namespace RegexMatchers {
    inline std::string filename() { return "[~]?[a-zA-Z0-9/._-]+\\.[a-zA-z0-9_]+"; }
    inline std::string c_variable() { return "[a-zA-Z]+[a-zA-Z_0-9]*"; }
    inline std::string dashed_word() { return "[a-zA-Z]+[\\-]+[a-zA-Z0-9\\-]*[a-zA-Z0-9]+"; }
    inline std::string alphabetic_word() { return "[a-zA-Z]+"; }
    inline std::string whitespace() { return "\\s"; }
    inline std::string double_quoted() { return "(\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\")"; }
    inline std::string single_quoted() { return "('[^'\\\\]*(?:\\\\.[^'\\\\]*)*')"; }
    inline std::string square_bracketed() { return "(\\[[^'\\\\]*(?:\\\\.[^'\\\\]*)*\\])"; }
    inline std::string integer() { return "-?[0-9]+(?![0-9\\.eE])"; }
    inline std::string number() { return "-?\\.?[0-9]+\\.?[0-9]*e?E?[-+]?[0-9]*"; }
}
}