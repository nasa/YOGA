#include <t-infinity/Demangle.h>
#include <RingAssertions.h>
#include <string>

using namespace inf;

TEST_CASE("Demangle a mangled symbol") {
    std::string mangled = "_Z7handlerv";
    auto demangled = demangle(mangled);

    REQUIRE("handler()" == demangled);
}

TEST_CASE("Extract a mangled symbol from a symbol line") {
    std::string symbol_line = "0   libinfinity.dylib                   0x00000001087742fd _Z7handlerv + 45";
    auto words = breakSymbolLineIntoWords(symbol_line);

    REQUIRE(6 == words.size());
    REQUIRE("0" == demangle(words[0]));
    REQUIRE("libinfinity.dylib" == demangle(words[1]));
    REQUIRE("0x00000001087742fd" == demangle(words[2]));
    REQUIRE("handler()" == demangle(words[3]));
    REQUIRE("+" == demangle(words[4]));
    REQUIRE("45" == demangle(words[5]));
}
