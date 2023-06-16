
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#include <RingAssertions.h>
#include <parfait/StringTools.h>

TEST_CASE("String split by delimiter") {
    std::string s = "This is a sentence.";
    auto l = Parfait::StringTools::split(s, " ");
    REQUIRE(l.size() == 4);
    REQUIRE("This" == l[0]);
    REQUIRE("is" == l[1]);
    REQUIRE("a" == l[2]);
    REQUIRE("sentence." == l[3]);
}

TEST_CASE("String split, but keep empty ones") {
    std::string s = "0,1,,,,5,9,";
    auto l = Parfait::StringTools::split(s, ",", false);
    REQUIRE(8 == l.size());
    REQUIRE("1" == l[1]);
    REQUIRE(l[2].empty());
    REQUIRE("9" == l[6]);
    REQUIRE(l[7].empty());
}

TEST_CASE("splitting by one delimiter should yield 2 items") {
    std::string s = "\n";
    auto l = Parfait::StringTools::split(s, "\n", false);
    REQUIRE(2 == l.size());
}

TEST_CASE("String split by string delimiter") {
    std::string s = "(>'-')> <('-'<) ^(' - ')^ <('-'<) (>'-')>";
    auto l = Parfait::StringTools::split(s, "'-'");
    REQUIRE(l.size() == 5);
}

TEST_CASE("Split with multiple spaces") {
    auto l = Parfait::StringTools::split("   facet normal  -0.470578     -0.335539      0.816070", " ");
    REQUIRE(l.size() == 5);
}

TEST_CASE("Split keep delimiter") {
    auto l = Parfait::StringTools::splitKeepDelimiter({"0:100:2"}, ":");
    REQUIRE(l.size() == 5);
}

TEST_CASE("Join as vector") {
    auto list = Parfait::StringTools::joinAsVector({"100", "110"}, ":");
    REQUIRE(list.size() == 3);
    REQUIRE(list[0] == "100");
    REQUIRE(list[1] == ":");
    REQUIRE(list[2] == "110");
}

TEST_CASE("Join strings") {
    std::vector<std::string> pikachu = {"pichu", "pikachu", "raichu"};
    std::string delimiter = "->";
    auto evolution = Parfait::StringTools::join(pikachu, delimiter);
    REQUIRE("pichu->pikachu->raichu" == evolution);
}

TEST_CASE("Strip extensions") {
    REQUIRE("pikachu" == Parfait::StringTools::stripExtension("pikachu.pokemon", "pokemon"));
    REQUIRE("pikachu" == Parfait::StringTools::stripExtension("pikachu.lb8.ugrid", "lb8.ugrid"));
    REQUIRE("pikachu" == Parfait::StringTools::stripExtension("pikachu.lb8.ugrid",
                                                              std::vector<std::string>{"lb8.ugrid", "pokemon"}));

    REQUIRE("pikachu" == Parfait::StringTools::stripExtension("pikachu.pokemon"));

    // Verify that relative paths are not lost
    REQUIRE("../pikachu" == Parfait::StringTools::stripExtension("../pikachu.pokemon"));
    REQUIRE("../pikachu" == Parfait::StringTools::stripExtension("../pikachu.ext", "ext"));
}

TEST_CASE("Find and replace all occurrences in a string") {
    std::string a = "a a a a a a a";
    auto b = Parfait::StringTools::findAndReplace(a, " ", "-");
    REQUIRE("a-a-a-a-a-a-a" == b);
}

TEST_CASE("Find and replace all occurrences of a substring in a string") {
    std::string a = "abc abc, abc + abc = f";
    auto b = Parfait::StringTools::findAndReplace(a, "abc", "pika");
    REQUIRE("pika pika, pika + pika = f" == b);
}

TEST_CASE("lstrip spaces") {
    std::string a = "  pika";
    auto b = Parfait::StringTools::lstrip(a);
    REQUIRE(b == "pika");
}

TEST_CASE("lstrip newline") {
    std::string a = "\npika";
    auto b = Parfait::StringTools::lstrip(a);
    REQUIRE(b == "pika");
}

TEST_CASE("rstrip whitespace") {
    std::string a = "pika   ";
    auto b = Parfait::StringTools::rstrip(a);
    REQUIRE(b == "pika");
}

TEST_CASE("rstrip whitespace not left whitespace") {
    std::string a = "  pika   ";
    auto b = Parfait::StringTools::rstrip(a);
    REQUIRE(b == "  pika");
}

TEST_CASE("strip both sides") {
    std::string a = "  pika  \n ";
    auto b = Parfait::StringTools::strip(a);
    REQUIRE(b == "pika");
}

TEST_CASE("downcase") {
    using namespace Parfait::StringTools;
    REQUIRE("pika" == tolower("pika"));
    REQUIRE("pika" == tolower("pIkA"));
    REQUIRE("pika" == tolower("PIKA"));
}

TEST_CASE("upcase") {
    using namespace Parfait::StringTools;
    REQUIRE("PIKA" == toupper("pika"));
    REQUIRE("PIKA" == toupper("pIkA"));
    REQUIRE("PIKA" == toupper("PIKA"));
}

TEST_CASE("is number") {
    using namespace Parfait::StringTools;
    REQUIRE(isInteger("1"));
    REQUIRE(isInteger("1234"));
    REQUIRE_FALSE(isInteger("--PreProcessor"));
}

TEST_CASE("is a double") {
    using namespace Parfait::StringTools;
    REQUIRE(isDouble("0.0"));
    REQUIRE(isDouble("0"));
    REQUIRE(isDouble("1e-4"));
    REQUIRE(isDouble(".87345"));
    REQUIRE_FALSE(isDouble("dog"));
    REQUIRE_FALSE(isDouble(" "));
    REQUIRE_FALSE(isDouble("\n"));
    REQUIRE_FALSE(isDouble("o"));

    // REQUIRE_FALSE(isDouble("D"));
    // REQUIRE_FALSE(isDouble("P"));
    // REQUIRE_FALSE(isDouble("p"));

    int count = 0;
    for (char letter : "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") {
        std::string s = {letter};
        if (isDouble(s)) count++;
    }
    REQUIRE(0 == count);
}

TEST_CASE("to integer") {
    using namespace Parfait::StringTools;
    REQUIRE(toInt("1") == 1);
    REQUIRE(toInt("1234") == 1234);
    REQUIRE_THROWS(toInt("--PreProcessor"));
}
TEST_CASE("get extension") {
    using namespace Parfait::StringTools;

    REQUIRE(getExtension("pika.chu") == "chu");
    REQUIRE(getExtension("pikachu") == "");
    REQUIRE(getExtension("pi.ka.chu") == "chu");
}
TEST_CASE("Handle ugrid extensions extra specially") {
    std::string ugrid_first_ext, ugrid_last_ext;
    SECTION("Can parse a typical lb8.ugrid") {
        std::string filename = "pikachu.lb8.ugrid";
        Parfait::StringTools::parseUgridFilename(filename, ugrid_first_ext, ugrid_last_ext);
        REQUIRE("pikachu" == filename);
        REQUIRE("lb8" == ugrid_first_ext);
        REQUIRE("ugrid" == ugrid_last_ext);
    }
    SECTION("Can parse a non-specified ugrid") {
        std::string filename = "pikachu.ugrid";
        Parfait::StringTools::parseUgridFilename(filename, ugrid_first_ext, ugrid_last_ext);
        REQUIRE("pikachu" == filename);
        REQUIRE("" == ugrid_first_ext);
        REQUIRE("ugrid" == ugrid_last_ext);
    }
    SECTION("Doesn't get confused by extra periods") {
        std::string filename = "pika.chu.ugrid";
        Parfait::StringTools::parseUgridFilename(filename, ugrid_first_ext, ugrid_last_ext);
        REQUIRE("pika.chu" == filename);
        REQUIRE("" == ugrid_first_ext);
        REQUIRE("ugrid" == ugrid_last_ext);
    }
}
TEST_CASE("Can recombine a ugrid name") {
    std::string base_name = "pikachu";
    std::string first_ext = "lb8";
    std::string last_ext = "ugrid";
    REQUIRE(Parfait::StringTools::combineUgridFilename(base_name, first_ext, last_ext) == "pikachu.lb8.ugrid");
}
TEST_CASE("contains") {
    using namespace Parfait::StringTools;
    REQUIRE(contains("pikachu", "pika"));
    REQUIRE_FALSE(contains("pikachu", "$"));
    REQUIRE_FALSE(contains("pika", "pikachu"));
}
TEST_CASE("Strip path") {
    using namespace Parfait::StringTools;
    REQUIRE(stripPath("../../dog") == "dog");
    REQUIRE(getPath("../../dog") == "../../");
    REQUIRE(getPath("dog") == "");
    REQUIRE(stripPath("dog") == "dog");
}

TEST_CASE("Erase whitespace only entries from vector of strings") {
    std::vector<std::string> vec = {"", "pika", " ", "chu!", "\n  "};
    auto clean = Parfait::StringTools::removeWhitespaceOnlyEntries(vec);
    REQUIRE(clean.size() == 2);
}
TEST_CASE("Erase whitespace only entries, don't mutate the rest") {
    std::vector<std::string> vec = {"", "pika chu!", " ", "charmander!", "\n  "};
    auto clean = Parfait::StringTools::removeWhitespaceOnlyEntries(vec);
    REQUIRE(clean[0] == "pika chu!");
}

TEST_CASE("remove comments") {
    auto s = Parfait::StringTools::removeCommentLines("# pika", "#");
    REQUIRE(s.empty());
}

TEST_CASE("don't remove not comments") {
    auto s = Parfait::StringTools::removeCommentLines("# pika\nchu", "#");
    REQUIRE(s == "chu");
}

TEST_CASE("generate string with random letters") {
    int n = 6;
    auto s = Parfait::StringTools::randomLetters(n);
    auto s2 = Parfait::StringTools::randomLetters(n);
    REQUIRE((int)s.size() == n);
    REQUIRE(s != s2);
}

TEST_CASE("stats with") {
    std::string a = "pikachu";
    REQUIRE(Parfait::StringTools::startsWith(a, "pika"));
}

TEST_CASE("ends with") {
    std::string a = "pikachu";
    REQUIRE(Parfait::StringTools::endsWith(a, "chu"));
}

TEST_CASE("wrap lines") {
    using namespace std;
    using namespace Parfait::StringTools;

    SECTION("split long word with dashes") {
        auto wrapped_lines = wrapLines("aabb", 2);
        REQUIRE(3 == wrapped_lines.size());
        REQUIRE("a-" == wrapped_lines[0]);
        REQUIRE("a-" == wrapped_lines[1]);
        REQUIRE("bb" == wrapped_lines[2]);
    }

    SECTION("split lines at spaces") {
        auto wrapped_lines = wrapLines("a dog", 3);
        REQUIRE(2 == wrapped_lines.size());
        REQUIRE("a" == wrapped_lines[0]);
        REQUIRE("dog" == wrapped_lines[1]);
    }

    SECTION("split a bunch of small words") {
        auto wrapped_lines = wrapLines("my dog is big", 6);
        REQUIRE(2 == wrapped_lines.size());
        REQUIRE("my dog" == wrapped_lines[0]);
        REQUIRE("is big" == wrapped_lines[1]);
    }

    SECTION("split big and small words") {
        auto wrapped_lines = wrapLines("charmander is a fire pokemon", 8);
        REQUIRE(4 == wrapped_lines.size());
        REQUIRE("charman-" == wrapped_lines[0]);
        REQUIRE("der is a" == wrapped_lines[1]);
        REQUIRE("fire" == wrapped_lines[2]);
        REQUIRE("pokemon" == wrapped_lines[3]);
    }
}

TEST_CASE("process white space in array list with slices") {
    {
        std::vector<std::string> cleaned_string_list = Parfait::StringTools::cleanupIntegerListString("0:10");
        REQUIRE(cleaned_string_list.size() == 3);
        REQUIRE(cleaned_string_list[0] == "0");
        REQUIRE(cleaned_string_list[1] == ":");
        REQUIRE(cleaned_string_list[2] == "10");
    }
    {
        std::vector<std::string> cleaned_string_list = Parfait::StringTools::cleanupIntegerListString("100:110");
        REQUIRE(cleaned_string_list.size() == 3);
        REQUIRE(cleaned_string_list[0] == "100");
        REQUIRE(cleaned_string_list[1] == ":");
        REQUIRE(cleaned_string_list[2] == "110");
    }

    {
        std::vector<std::string> cleaned_string_list = Parfait::StringTools::cleanupIntegerListString(" 100 :110  ");
        REQUIRE(cleaned_string_list.size() == 3);
        REQUIRE(cleaned_string_list[0] == "100");
        REQUIRE(cleaned_string_list[1] == ":");
        REQUIRE(cleaned_string_list[2] == "110");
    }

    {
        std::vector<std::string> cleaned_string_list =
            Parfait::StringTools::cleanupIntegerListString("6,7,8,100:110,500 ");
        REQUIRE(cleaned_string_list.size() == 7);
        REQUIRE(cleaned_string_list[0] == "6");
        REQUIRE(cleaned_string_list[1] == "7");
        REQUIRE(cleaned_string_list[2] == "8");
        REQUIRE(cleaned_string_list[3] == "100");
        REQUIRE(cleaned_string_list[4] == ":");
        REQUIRE(cleaned_string_list[5] == "110");
        REQUIRE(cleaned_string_list[6] == "500");
    }
}

TEST_CASE("Get integer list from slice operator") {
    SECTION("starting at 0") {
        std::vector<int> my_array = Parfait::StringTools::parseIntegerList("0:10");
        REQUIRE(my_array.size() == 11);
        for (int i = 0; i < 11; i++) {
            REQUIRE(my_array[i] == i);
        }
    }

    SECTION("starting at not zero") {
        std::vector<int> my_array = Parfait::StringTools::parseIntegerList("100:110");
        REQUIRE(my_array.size() == 11);
        for (int i = 0; i < 11; i++) {
            REQUIRE(my_array[i] == i + 100);
        }
    }

    SECTION("with spaces in the query") {
        std::vector<int> my_array = Parfait::StringTools::parseIntegerList(" 100 :110  ");
        REQUIRE(my_array.size() == 11);
        for (int i = 0; i < 11; i++) {
            REQUIRE(my_array[i] == i + 100);
        }
    }
    SECTION("more after a slice") {
        auto my_array = Parfait::StringTools::parseIntegerList("6:8 10:12 500");
        REQUIRE(my_array == std::vector<int>{6, 7, 8, 10, 11, 12, 500});
    }
    SECTION("Also allow comma separated") {
        std::vector<int> my_array = Parfait::StringTools::parseIntegerList("6,7,8,100:105,500");
        REQUIRE(my_array == std::vector<int>{6, 7, 8, 100, 101, 102, 103, 104, 105, 500});
    }
    SECTION("Also allow negative numbers") {
        std::vector<int> my_array = Parfait::StringTools::parseIntegerList("6,7,8,-2:2,500");
        REQUIRE(my_array == std::vector<int>{6, 7, 8, -2, -1, 0, 1, 2, 500});
    }
    SECTION("Can use newline") {
        std::string my_string = "1\n5\n8";
        std::vector<int> my_array = Parfait::StringTools::parseIntegerList("1\n5\n8");
        REQUIRE(my_array == std::vector<int>{1, 5, 8});
    }
}

TEST_CASE("Can get an array of quoted phrases") {
    std::string quoted_input_array = R"("x" "dy" "y<sup>+</sup>" "u<sub><greek>t</greek></sub> [m/s]")";
    auto phrases = Parfait::StringTools::parseQuotedArray(quoted_input_array);
    REQUIRE(phrases.size() == 4);
}

TEST_CASE("Pad string for easy formatting") {
    std::string t = "1234";
    SECTION("right pad") {
        t = Parfait::StringTools::rpad(t, 10, ".");
        REQUIRE(t.size() == 10);
        REQUIRE("1234......" == t);
    }
    SECTION("left pad") {
        t = Parfait::StringTools::lpad(t, 10, ".");
        REQUIRE(t.size() == 10);
        REQUIRE("......1234" == t);
    }
}

TEST_CASE("Can easily remove substring from beginning") {
    std::string s = "cachedpikachu";
    auto p = Parfait::StringTools::stripFromBeginning(s, "cached");
    REQUIRE(p == "pikachu");
}