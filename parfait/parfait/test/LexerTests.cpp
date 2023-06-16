#include <RingAssertions.h>
#include <parfait/Lexer.h>
#include <parfait/JsonParser.h>

bool is_match(const std::string& s, const std::vector<std::string>& tokens) {
    auto result = Parfait::StringTools::join(tokens, ",");
    if (s != result) printf("Expected: %s \nActual:   %s", s.c_str(), result.c_str());
    return s == result;
}

using namespace Parfait;

TEST_CASE("create lexer") {
    SECTION("identify plain tokens") {
        Lexer::TokenMap token_map;
        token_map.push_back({Lexer::Matchers::whitespace(), ""});
        token_map.push_back({Lexer::Matchers::newline(), ""});
        token_map.push_back({"{", "O"});
        token_map.push_back({"}", "C"});

        Lexer lexer(token_map);

        REQUIRE(is_match("", lexer.extractStrings(lexer.tokenize(""))));
        REQUIRE(is_match("", lexer.extractStrings(lexer.tokenize("   "))));
        REQUIRE_THROWS(lexer.tokenize("!"));
        REQUIRE(is_match("O", lexer.extractStrings(lexer.tokenize("{"))));
        REQUIRE(is_match("O,C", lexer.extractStrings(lexer.tokenize("{\n\n}"))));
        REQUIRE(is_match("O,C", lexer.extractStrings(lexer.tokenize("{ }"))));
    }

    SECTION("whitespace matching") {
        Lexer::TokenMap token_map;
        token_map.push_back({Lexer::Matchers::newline(), "N"});
        token_map.push_back({Lexer::Matchers::whitespace(), "W"});
        Lexer lexer(token_map);

        REQUIRE(is_match("W,N,N,W,W,N", lexer.extractStrings(lexer.tokenize(" \n\n  \n"))));
    }

    SECTION("premade matchers") {
        SECTION("filename") {
            Lexer::TokenMap token_map;
            token_map.push_back({Lexer::Matchers::filename(), "FN:R"});
            Lexer lexer(token_map);

            REQUIRE(is_match("FN:dog.txt", lexer.extractStrings(lexer.tokenize("dog.txt"))));
            REQUIRE(is_match("FN:Dog.txt", lexer.extractStrings(lexer.tokenize("Dog.txt"))));
            REQUIRE(is_match("FN:Dog.map_bc", lexer.extractStrings(lexer.tokenize("Dog.map_bc"))));
            REQUIRE(is_match("FN:../Dog.txt", lexer.extractStrings(lexer.tokenize("../Dog.txt"))));
            REQUIRE(is_match("FN:~/yard/Dog.txt", lexer.extractStrings(lexer.tokenize("~/yard/Dog.txt"))));
            REQUIRE(is_match("FN:pokemon_names.json", lexer.extractStrings(lexer.tokenize("pokemon_names.json"))));
            REQUIRE(is_match("FN:pokemon-names.json", lexer.extractStrings(lexer.tokenize("pokemon-names.json"))));
            REQUIRE(is_match("FN:pokemon_names2.json", lexer.extractStrings(lexer.tokenize("pokemon_names2.json"))));
            REQUIRE(is_match("FN:pikachu.m4", lexer.extractStrings(lexer.tokenize("pikachu.m4"))));
            REQUIRE(is_match("FN:/projects/dog.txt", lexer.extractStrings(lexer.tokenize("/projects/dog.txt"))));
        }
        SECTION("variable name") {
            Lexer::TokenMap token_map;
            token_map.push_back({Lexer::Matchers::whitespace(), ""});
            token_map.push_back({Lexer::Matchers::c_variable(), "R"});
            Lexer lexer(token_map);

            REQUIRE(is_match("x,y,z", lexer.extractStrings(lexer.tokenize("x y z"))));
            REQUIRE(is_match("stuff", lexer.extractStrings(lexer.tokenize("stuff"))));
            REQUIRE(is_match("moreStuff", lexer.extractStrings(lexer.tokenize("moreStuff"))));
            REQUIRE(is_match("more_stuff", lexer.extractStrings(lexer.tokenize("more_stuff"))));
            REQUIRE(is_match("more_stuff2", lexer.extractStrings(lexer.tokenize("more_stuff2"))));
            REQUIRE_THROWS(lexer.tokenize("1stuff"));
        }

        SECTION("dashed-words") {
            Lexer::TokenMap token_map;
            token_map.push_back({Lexer::Matchers::whitespace(), ""});
            token_map.push_back({Lexer::Matchers::dashed_word(), "DW:R"});
            token_map.push_back({Lexer::Matchers::c_variable(), "C:R"});
            Lexer lexer(token_map);

            REQUIRE(is_match("DW:dashed-word,DW:another-one",
                             lexer.extractStrings(lexer.tokenize("dashed-word another-one"))));
            REQUIRE(is_match("DW:dashed-word,DW:a-longer-one",
                             lexer.extractStrings(lexer.tokenize("dashed-word a-longer-one"))));
            REQUIRE(is_match("C:word,C:c_var", lexer.extractStrings(lexer.tokenize("word c_var"))));
        }

        SECTION("bracketed") {
            Lexer::TokenMap token_map;
            token_map.push_back({Lexer::Matchers::square_bracketed(), "BS:R"});
            Lexer lexer(token_map);
            REQUIRE(is_match("BS:[Some crazy @*&^%4 string!~]",
                             lexer.extractStrings(lexer.tokenize("[Some crazy @*&^%4 string!~]"))));
        }

        SECTION("quoted") {
            Lexer::TokenMap token_map;
            token_map.push_back({Lexer::Matchers::double_quoted(), "##R"});
            token_map.push_back({Lexer::Matchers::single_quoted(), "#R"});
            Lexer lexer(token_map);

            REQUIRE(is_match("##\"Some crazy @*&^%4 string!~\"",
                             lexer.extractStrings(lexer.tokenize("\"Some crazy @*&^%4 string!~\""))));
            REQUIRE(is_match("##\"Some 'crazy' @*&^%4 string!~\"",
                             lexer.extractStrings(lexer.tokenize("\"Some 'crazy' @*&^%4 string!~\""))));
            REQUIRE(is_match("#'Some crazy @*&^%4 string!~'",
                             lexer.extractStrings(lexer.tokenize("'Some crazy @*&^%4 string!~'"))));
            REQUIRE(is_match("#'Some \"crazy\" @*&^%4 string!~'",
                             lexer.extractStrings(lexer.tokenize("'Some \"crazy\" @*&^%4 string!~'"))));

            REQUIRE(is_match("#'Two',#'Strings'", lexer.extractStrings(lexer.tokenize("'Two''Strings'"))));
            REQUIRE(is_match("##\"Two\",##\"Strings\"", lexer.extractStrings(lexer.tokenize("\"Two\"\"Strings\""))));

            REQUIRE(is_match("##\"Some \\\"crazy\\\" @*&^%4\\ \t string!~\"",
                             lexer.extractStrings(lexer.tokenize("\"Some \\\"crazy\\\" @*&^%4\\ \t string!~\""))));
            REQUIRE(is_match("#\'Some \\\'crazy\\\' @*&^%4\\ \t string!~\'",
                             lexer.extractStrings(lexer.tokenize("\'Some \\\'crazy\\\' @*&^%4\\ \t string!~\'"))));
        }

        SECTION("numbers") {
            SECTION("match integers and floating point separately") {
                Lexer::TokenMap token_map;
                token_map.push_back({Lexer::Matchers::whitespace(), ""});
                token_map.push_back({Lexer::Matchers::integer(), "IR"});
                token_map.push_back({Lexer::Matchers::number(), "FR"});
                Lexer lexer(token_map);

                REQUIRE(is_match("F2.000e+20", lexer.extractStrings(lexer.tokenize("2.000e+20"))));
                REQUIRE(is_match("F2000.0", lexer.extractStrings(lexer.tokenize("2000.0"))));
                REQUIRE(
                    is_match("I4,I5,I6,F.2,F4.5,F1e+16", lexer.extractStrings(lexer.tokenize("4 5 6 .2 4.5 1e+16"))));
            }
            SECTION("any number") {
                Lexer::TokenMap token_map;
                token_map.push_back({Lexer::Matchers::whitespace(), ""});
                token_map.push_back({Lexer::Matchers::number(), "R"});
                token_map.push_back({",", "COM"});
                Lexer lexer(token_map);

                REQUIRE(is_match("0,1,2.234,-1.0,1e-16,COM,1.5E-12",
                                 lexer.extractStrings(lexer.tokenize("0 1 2.234 -1.0 1e-16, 1.5E-12"))));
            }
        }

        SECTION("comment lines") {
            Lexer::TokenMap token_map;
            token_map.push_back({Lexer::Matchers::newline(), ""});
            token_map.push_back({Lexer::Matchers::comment("#"), "R"});
            token_map.push_back({Lexer::Matchers::comment("//"), "R"});
            Lexer lexer(token_map);

            REQUIRE(is_match("#this is a comment", lexer.extractStrings(lexer.tokenize("#this is a comment"))));
            REQUIRE(
                is_match("#comment 1,//comment 2", lexer.extractStrings(lexer.tokenize("#comment 1\n//comment 2"))));
        }

        SECTION("advanced comments") {
            Lexer::TokenMap token_map;
            token_map.push_back({Lexer::Matchers::whitespace(), ""});
            token_map.push_back({Lexer::Matchers::newline(), ""});
            token_map.push_back({Lexer::Matchers::c_variable(), "R"});
            token_map.push_back({Lexer::Matchers::comment("!"), "#R"});
            Lexer lexer(token_map);

            REQUIRE(is_match("x,y,z,#! comment after some stuff",
                             lexer.extractStrings(lexer.tokenize("x y z ! comment after some stuff"))));
        }
    }

    SECTION("identify regular expression tokens and allow find/replace for characters") {
        Lexer::TokenMap token_map;
        token_map.push_back({"R\"[a-z]+\"", "R\"=#"});

        Lexer lexer(token_map);

        REQUIRE(is_match("#name#", lexer.extractStrings(lexer.tokenize("\"name\""))));
    }

    SECTION("identify regular expression tokens and allow deletion of characters") {
        Lexer::TokenMap token_map;
        token_map.push_back({"R\"[a-z]+\"", "--R\"="});

        Lexer lexer(token_map);
        std::string dog = "\"dog\"";
        REQUIRE(is_match("--name", lexer.extractStrings(lexer.tokenize("\"name\""))));
    }

    SECTION("specify json syntax") {
        Lexer lexer(JsonParser::jsonSyntax());
        std::string json = "{\"preprocessor\":\"NC_PreProcessor\",";
        json += "\"fluid solver\": \"HyperSolve\",";
        json += "\"absolute tolerance\": 1e-16,";
        json += "    \"HyperSolve\":{\n";
        json += "        \"states\":{\n";
        json += "            \"farfield\":{\n";
        json += "                \"densities\":[0.001],\n";
        json += "                \"speed\":2000.0,\n";
        json += "                \"temperature\":288.15\n";
        json += "            },\n";
        json += "            \"initial\":{\n";
        json += "                \"densities\":[0.001],\n";
        json += "                \"speed\":2000.0,\n";
        json += "                \"temperature\":1.0\n";
        json += "            }\n";
        json += "        },";
        json += "\"an integer\":9001";
        json += "}";
        std::string expected = "BO,#preprocessor#,COL,#NC_PreProcessor#,COM,";
        expected += "#fluid solver#,COL,#HyperSolve#,COM,";
        expected += "#absolute tolerance#,COL,F1e-16,COM,";
        expected += "#HyperSolve#,COL,BO,";
        expected += "#states#,COL,BO,";
        expected += "#farfield#,COL,BO,";
        expected += "#densities#,COL,BA,F0.001,EA,COM,";
        expected += "#speed#,COL,F2000.0,COM,";
        expected += "#temperature#,COL,F288.15,EO,COM,";
        expected += "#initial#,COL,BO,";
        expected += "#densities#,COL,BA,F0.001,EA,COM,";
        expected += "#speed#,COL,F2000.0,COM,";
        expected += "#temperature#,COL,F1.0,";
        expected += "EO,";
        expected += "EO,";
        expected += "COM,#an integer#,COL,I9001,";
        expected += "EO";
        auto actual = lexer.tokenize(json);

        REQUIRE(is_match(expected, lexer.extractStrings(actual)));
    }
}
