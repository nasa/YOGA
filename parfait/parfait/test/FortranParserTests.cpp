#include <RingAssertions.h>
#include <fstream>
#include <parfait/FortranSyntax.h>

using namespace Parfait;

std::string sourceFileToString(const std::string& filename) {
    using namespace std;
    ifstream t(filename);
    bool file_exists = t.good();
    std::string source;
    if (file_exists) source = string((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
    return source;
}

int count(const std::vector<Lexer::Token>& tokens, const std::string& value) {
    int n = 0;
    for (auto& t : tokens)
        if (t.to_string() == value) n++;
    return n;
}

TEST_CASE("tokenize stream of fortran code") {
    Lexer lexer(FortranSyntax::syntax());
    SECTION("empty string has no tokens") {
        auto tokens = lexer.tokenize("");
        REQUIRE(0 == tokens.size());
    }

    SECTION("shortest possible fortran program has one token") {
        auto tokens = lexer.tokenize("end");
        REQUIRE(1 == tokens.size());
        REQUIRE(1 == tokens.front().lineNumber());
        REQUIRE(1 == tokens.front().column());
        REQUIRE(tokens.front().to_string() == FortranSyntax::end());
    }

    SECTION("empty named program has tokens") {
        std::string s = "program dog\n";
        s += "end program dog";
        auto tokens = lexer.tokenize(s);
        REQUIRE(2 == count(tokens, FortranSyntax::program()));
        REQUIRE(1 == count(tokens, FortranSyntax::newline()));
        REQUIRE(6 == tokens.size());

        REQUIRE(2 == tokens.back().lineNumber());
        REQUIRE(13 == tokens.back().column());

        REQUIRE(tokens[0].to_string() == FortranSyntax::program());
    }

    SECTION("handle comment lines") {
        std::string s = "begin program\n! this program is empty\nend program";
        auto tokens = lexer.tokenize(s);
    }

    SECTION("handle module use statements") {
        std::string s = "module dog\nuse grid, only : node_count, &\ncell_count\nend module";
        auto tokens = lexer.tokenize(s);
        REQUIRE(tokens[0].to_string() == FortranSyntax::module());
    }

    SECTION("handle variable declarations") {
        std::string s = "module dog\n";
        s += "logical :: is_red = .false.\n";
        s += "integer(i8) :: pointer : ptr => null()\n";
        s += "character(len=*), intent(in)    :: name";
        s += "end module";
        auto tokens = lexer.tokenize(s);
        REQUIRE(tokens[3].to_string() == FortranSyntax::logical());
        REQUIRE(tokens[4].to_string() == FortranSyntax::doubleColon());
        REQUIRE(tokens[6].to_string() == FortranSyntax::equals());
        REQUIRE(tokens[7].to_string() == FortranSyntax::dotfalse());
        REQUIRE(tokens[8].to_string() == FortranSyntax::newline());
        REQUIRE(tokens[10].to_string() == FortranSyntax::openParentheses());
        REQUIRE(tokens[12].to_string() == FortranSyntax::closeParentheses());
        REQUIRE(tokens[13].to_string() == FortranSyntax::doubleColon());
        REQUIRE(tokens[15].to_string() == FortranSyntax::colon());
        REQUIRE(tokens[17].to_string() == FortranSyntax::associate());
        REQUIRE(tokens[26].to_string() == FortranSyntax::star());
    }

    SECTION("member value access") {
        std::string s = "dog%name = \"ralph\"\n";
        s += "dog%name = 'ralph'\n";
        s += "dog%speed = 3.14 + 14";
        s += "dog%speed = 3.14 - 14";
        auto tokens = lexer.tokenize(s);
        REQUIRE(4 == count(tokens, FortranSyntax::percent()));
        REQUIRE(1 == count(tokens, FortranSyntax::plus()));
        REQUIRE(1 == count(tokens, FortranSyntax::minus()));
    }

    SECTION("boolean statements") {
        std::string s = "if(a /= dog%speed) then\n";
        auto tokens = lexer.tokenize(s);
        REQUIRE(tokens[0].to_string() == FortranSyntax::If());
        REQUIRE(tokens[3].to_string() == FortranSyntax::notEquals());
    }

    SECTION("using integers and booleans without spaces like savages") {
        std::string s = "if(a<0.and.b>5.) then";
        auto tokens = lexer.tokenize(s);
        REQUIRE(tokens[5].to_string() == FortranSyntax::dotand());
    }

    SECTION("multiple statements per line") {
        std::string s = "a = 5.0; b = 1.0_dp";
        auto tokens = lexer.tokenize(s);
        REQUIRE(tokens[3].to_string() == FortranSyntax::semicolon());
        REQUIRE(tokens[7].to_string() == FortranSyntax::underscore());
    }

    SECTION("math") {
        std::string s = "a = b/x";
        auto tokens = lexer.tokenize(s);
        REQUIRE(tokens[3].to_string() == FortranSyntax::divide());
    }

    SECTION("comparators") {
        std::string s = "if(a > b .and. .not. b < c .or. b == d) then";
        auto tokens = lexer.tokenize(s);
        REQUIRE(tokens[3].to_string() == FortranSyntax::greater());
        REQUIRE(tokens[6].to_string() == FortranSyntax::dotnot());
        REQUIRE(tokens[8].to_string() == FortranSyntax::less());
        REQUIRE(tokens[10].to_string() == FortranSyntax::dotor());
        REQUIRE(tokens[12].to_string() == FortranSyntax::doubleEquals());
    }

    SECTION("string operations") {
        std::string s = "write(*,*) 'dog' // trim(name)";
        auto tokens = lexer.tokenize(s);
        REQUIRE(tokens[7].to_string() == FortranSyntax::concatination());
    }
}
