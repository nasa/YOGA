#include <RingAssertions.h>
#include <parfait/Dictionary.h>
#include <parfait/JsonParser.h>
#include <sstream>

using namespace Parfait;

TEST_CASE("empty dict object") {
    Dictionary dict;
    REQUIRE(0 == dict.size());
    REQUIRE(Dictionary::Object == dict.type());
    REQUIRE_THROWS(dict.asString());
    REQUIRE_THROWS(dict.asInt());
    REQUIRE_THROWS(dict.asDouble());
    REQUIRE("{}" == dict.dump());
}

TEST_CASE("add boolean") {
    Dictionary dict;
    dict["is available"] = false;
    dict["truth"] = true;
    REQUIRE(Dictionary::Boolean == dict["is available"].type());
    REQUIRE_FALSE(dict["is available"].asBool());
    REQUIRE(dict["truth"].asBool());
    REQUIRE("{\"is available\":false,\"truth\":true}" == dict.dump());
}

TEST_CASE("add a string") {
    Dictionary dict;
    dict["name"] = "pikachu";
    REQUIRE(1 == dict.count("name"));
    REQUIRE(dict.has("name"));
    REQUIRE(1 == dict.size());
    REQUIRE(Dictionary::String == dict["name"].type());
    REQUIRE("pikachu" == dict.at("name").asString());
    REQUIRE("{\"name\":\"pikachu\"}" == dict.dump());
}

TEST_CASE("add a number") {
    Dictionary dict;
    dict["shoe size"] = 7;
    REQUIRE(1 == dict.size());
    REQUIRE(Dictionary::Integer == dict["shoe size"].type());
    REQUIRE("7" == dict.at("shoe size").asString());
    REQUIRE(7 == dict.at("shoe size").asInt());
    REQUIRE("{\"shoe size\":7}" == dict.dump());

    dict["power"] = 3.14;
    REQUIRE(2 == dict.size());
    REQUIRE(Dictionary::Double == dict["power"].type());
    REQUIRE("3.14" == dict.at("power").asString().substr(0, 4));

    dict["really small"] = 1e-12;
    REQUIRE(1.0e-12 == dict["really small"].asDouble());

    dict["really accurate"] = -1.17904969363339;
    REQUIRE(-1.17904969363339 == dict["really accurate"].asDouble());
}

TEST_CASE("add null") {
    Dictionary dict;
    dict["null-pokemon"] = Dictionary::null();
    REQUIRE(Dictionary::TYPE::Null == dict["null-pokemon"].type());
}

TEST_CASE("empty array") {
    Dictionary dict;
    dict["empty array"] = std::vector<int>();
    REQUIRE(1 == dict.size());
    REQUIRE(0 == dict["empty array"].size());
    REQUIRE("{\"empty array\":[]}" == dict.dump());
}

TEST_CASE("add a vector") {
    Dictionary dict;

    std::vector<int> v{0, 1, 2};
    dict["some integers"] = v;
    REQUIRE(1 == dict.size());
    REQUIRE(Dictionary::Object == dict.type());
    REQUIRE(3 == dict["some integers"].size());
    REQUIRE(Dictionary::IntArray == dict["some integers"].type());

    auto v2 = dict.at("some integers").asInts();
    REQUIRE(v == v2);
    REQUIRE("{\"some integers\":[0,1,2]}" == dict.dump());

    std::vector<double> my_doubles{3.14, 3.14};
    dict["my doubles"] = my_doubles;
    REQUIRE(2 == dict["my doubles"].size());
    REQUIRE(Dictionary::DoubleArray == dict["my doubles"].type());
    auto d = dict.at("my doubles").asDoubles();
    CHECK(my_doubles == d);

    std::vector<std::string> some_strings{"array", "of", "strings"};
    dict["some strings"] = some_strings;
    REQUIRE(3 == dict["some strings"].size());
    REQUIRE(Dictionary::StringArray == dict["some strings"].type());
    auto s = dict.at("some strings").asStrings();
    REQUIRE(some_strings == s);

    std::vector<bool> my_bools{true, false, true};
    dict["some bools"] = my_bools;
    REQUIRE(4 == dict.size());
    REQUIRE(3 == dict["some bools"].size());
    REQUIRE(Dictionary::BoolArray == dict["some bools"].type());
    REQUIRE(my_bools == dict.at("some bools").asBools());
}

TEST_CASE("Add nested elements") {
    Dictionary dict;
    dict["pokemon"]["name"] = "pikachu";
    std::vector<int> toes{3, 3, 4, 4};
    dict["pokemon"]["toes per leg"] = toes;
    REQUIRE(1 == dict.size());
    REQUIRE("pikachu" == dict.at("pokemon").at("name").asString());
    REQUIRE(toes == dict.at("pokemon").at("toes per leg").asInts());
    REQUIRE("{\"pokemon\":{\"name\":\"pikachu\",\"toes per leg\":[3,3,4,4]}}" == dict.dump());
}

TEST_CASE("add arrays via [] operators") {
    Dictionary dict;
    dict["some array"][0] = 5;
    dict["some array"][1] = 6;
    REQUIRE(2 == dict["some array"].size());
    REQUIRE(5 == dict["some array"][0].asInt());
    REQUIRE(6 == dict["some array"][1].asInt());
    REQUIRE("{\"some array\":[5,6]}" == dict.dump());

    std::vector<int> expected = {5, 6};
    std::vector<int> actual = dict["some array"].asInts();

    REQUIRE(expected == actual);
}

TEST_CASE("Scalar Integers can be converted to arrays of single length") {
    Parfait::Dictionary dict;
    dict["some number"] = 6;
    REQUIRE(6 == dict["some number"].asInt());
    REQUIRE(std::vector<int>{6} == dict["some number"].asInts());
    REQUIRE(6.0 == dict["some number"].asDouble());
    REQUIRE(std::vector<double>{6.0} == dict["some number"].asDoubles());
    dict["not a number"] = "my string";
    REQUIRE_THROWS(dict["not a number"].asInts());
}

TEST_CASE("Scalar Doubles can be converted to arrays of single length") {
    Parfait::Dictionary dict;
    dict["some number"] = 6.0;
    REQUIRE(6.0 == dict["some number"].asInt());
    REQUIRE(std::vector<double>{6.0} == dict["some number"].asDoubles());
    dict["not a number"] = "my string";
    REQUIRE_THROWS(dict["not a number"].asDoubles());
}

TEST_CASE("Scalar Bools can be converted to arrays of single length") {
    Parfait::Dictionary dict;
    dict["some"] = true;
    REQUIRE(true == dict["some"].asBool());
    REQUIRE(std::vector<bool>{true} == dict["some"].asBools());
    dict["not an bool"] = "my string";
    REQUIRE_THROWS(dict["not an array"].asBools());
}

TEST_CASE("Scalar Strings can be converted to arrays of single length") {
    Parfait::Dictionary dict;
    dict["some"] = "some";
    REQUIRE("some" == dict["some"].asString());
    REQUIRE(std::vector<std::string>{"some"} == dict["some"].asStrings());
    dict["not an string"] = 6;
    REQUIRE_THROWS(dict["not an array"].asStrings());
}

TEST_CASE("Scalar Objects can be converted to arrays of single length") {
    Parfait::Dictionary dict;
    Parfait::Dictionary nested_dict;
    nested_dict["nested"] = 6;
    dict["some"] = nested_dict;
    auto obj_array = dict["some"].asObjects();
    REQUIRE(obj_array.size() == 1);
    REQUIRE(6 == obj_array[0]["nested"].asInt());
    dict["not an object"] = 6;
    REQUIRE_THROWS(dict["not an object"].asObjects());
}

TEST_CASE("Demonstrate how to get a Json object from a dict formatted string") {
    SECTION("Object") {
        std::string settings_string =
            R"({"pokemon":{"name":"Pikachu", "nicknames":["pika", "pikachu", "yellow rat"]}})";
        auto dict_object = Parfait::JsonParser::parse(settings_string);
        REQUIRE(dict_object.at("pokemon").at("name").asString() == "Pikachu");
    }
    SECTION("Array of objects") {
        std::string settings_string = R"([{"name":"Pikachu"}, {"name":"Mewtwo"}])";
        auto dict_object = Parfait::JsonParser::parse(settings_string);
        REQUIRE(2 == dict_object.size());
        REQUIRE(dict_object.at(0).at("name").asString() == "Pikachu");
        REQUIRE(dict_object.at(1).at("name").asString() == "Mewtwo");
    }
}

TEST_CASE("Pretty print Json") {
    Dictionary dict;
    dict["some array"][0] = 5;
    dict["some array"][1] = 6;
    dict["some value"] = double(1e200);
    dict["empty array"] = std::vector<double>{};
    dict["empty object"] = Dictionary();
    std::string expected = R"({
    "empty array": [],
    "empty object": {},
    "some array": [
        5,
        6
    ],
    "some value": 1e+200
})";
    REQUIRE(expected == dict.dump(4));
}

TEST_CASE("Prevent precision issues") {
    std::string c = R"({"object w/array that get reparsed with int + double members":{"array":[1.0,1.0e+200]}})";
    auto dict = Parfait::JsonParser::parse(c);
    auto reparse = Parfait::JsonParser::parse(dict.dump());
    REQUIRE_NOTHROW(reparse.dump(4));
    INFO(reparse.dump(4));
    const auto& array = dict.at("object w/array that get reparsed with int + double members").at("array");
    REQUIRE(array.type() == Dictionary::DoubleArray);
}

TEST_CASE("Can merge two JSON objects into a new JSON object") {
    using namespace Parfait;
    std::string defaults = R"({
    "nested object1": {
        "prop1": "default value",
        "prop2": "default value"
    },
    "some array": [
        5,
        6
    ],
    "some value": 1.0
})";
    std::string o = R"({
    "nested object1": {
        "prop2": "new value"
    },
    "some value": -12.0
})";
    Dictionary merged = (JsonParser::parse(defaults)).overrideEntries(JsonParser::parse(o));
    REQUIRE("default value" == merged.at("nested object1").at("prop1").asString());
    REQUIRE("new value" == merged.at("nested object1").at("prop2").asString());
    REQUIRE(-12.0 == Approx(merged.at("some value").asDouble()));
}

TEST_CASE("Can iterate through objects") {
    using namespace Parfait;
    std::string config = R"({
    "regions": [
        {"type": "sphere", "radius":1.0, "center":[0,0,0]},
        {"type": "sphere", "radius":1.5, "center":[1,0,0]}
    ]
})";

    auto dict = JsonParser::parse(config);
    REQUIRE(dict.keys().size() == 1);

    for (auto r : dict.at("regions").asObjects()) {
        printf("%s\n", r.dump().c_str());
    }
}

TEST_CASE("Can assign a dictionary to a string") {
    std::string config = R"([
        {"type": "sphere", "radius":1.0, "center":[0,0,0]},
        {"type": "sphere", "radius":1.5, "center":[1,0,0]}
    ])";
    auto dict = JsonParser::parse(config);
    dict = "dummy";
    REQUIRE(0 == dict.size());
}

TEST_CASE("Can easily check if something exists and is true") {
    Parfait::Dictionary dict;
    REQUIRE_FALSE(dict.isTrue("invalid key"));

    dict["int key"] = 1;
    REQUIRE_FALSE(dict.isTrue("int key"));

    dict["bool key"] = false;
    REQUIRE_FALSE(dict.at("bool key").asBool());
    REQUIRE_FALSE(dict.isTrue("bool key"));

    dict["actually true"] = true;
    REQUIRE(dict.at("actually true").asBool());
    REQUIRE(dict.isTrue("actually true"));
}

TEST_CASE("Give available options if the user fails to request a valid key") {
    Parfait::Dictionary dict;
    dict["dog"] = 1;
    REQUIRE_THROWS_WITH(dict.at("cat"), Contains("dog"));
}

TEST_CASE("Can use _dict literal") {
    using namespace Parfait::json_literals;
    auto d = R"({"dog":1})"_json;
    REQUIRE(d.has("dog"));
    REQUIRE(d.at("dog").asInt() == 1);
}

TEST_CASE("Can delete entry from dictionary") {
    Parfait::Dictionary dict;
    dict["dog"] = 1;
    REQUIRE(dict.has("dog"));
    dict.erase("dog");
    REQUIRE(not dict.has("dog"));
}

TEST_CASE("Can overwrite dictionary with self") {
    Parfait::Dictionary dict;
    dict["dog"]["color"] = "brown";
    dict = dict["dog"];
    REQUIRE(dict.has("color"));
    REQUIRE(dict.at("color").asString() == "brown");
}

TEST_CASE("Can print dictionary via ostream") {
    Parfait::Dictionary dict;
    dict["color"] = "brown";
    std::stringstream ss;
    ss << dict;
    REQUIRE("{\"color\":\"brown\"}" == ss.str());
}