#include <RingAssertions.h>
#include <MessagePasser/MessagePasser.h>

TEST_CASE("serialize a vector of bools") {
    std::vector<bool> bools{true, true, false};
    MessagePasser::Message msg;
    msg.pack(bools);
    msg.finalize();

    SECTION("Can unpack a vector of bools") {
        std::vector<bool> more_bools(false, 500);
        msg.unpack(more_bools);
        REQUIRE(3 == more_bools.size());
        REQUIRE(more_bools[0]);
        REQUIRE(more_bools[1]);
        REQUIRE_FALSE(more_bools[2]);
    }

    SECTION("message is of correct size") { REQUIRE(msg.size() == 7); }
}

TEST_CASE("serialize a string") {
    MessagePasser::Message msg;
    std::string s = "Hello, there!";
    msg.pack(s);
    msg.pack("literally!");
    std::string s2;
    msg.unpack(s2);
    REQUIRE(s == s2);
    std::string s3;
    msg.unpack(s3);
    REQUIRE("literally!" == s3);
}

TEST_CASE("Throw exception if trying to unpack beyond end") {
    std::vector<float> some_floats(5);
    MessagePasser::Message msg(some_floats);

    std::vector<double> some_doubles;
    REQUIRE_THROWS(msg.unpack(some_doubles));
}

TEST_CASE("Make sure non-trivially copyables cause compiler error") {
    struct Dog {
        std::vector<int> treats;
        std::vector<float> times_to_sit_still;
    };

    Dog dog;
    dog.treats = {5};
    dog.times_to_sit_still = {.1, .2};

    auto dog_packer = [](MessagePasser::Message& msg, const Dog& d) -> void {
        msg.pack(d.treats);
        msg.pack(d.times_to_sit_still);
    };

    MessagePasser::Message msg(dog, dog_packer);

    auto dog_unpacker = [](MessagePasser::Message& msg, Dog& d) -> void {
        msg.unpack(d.treats);
        msg.unpack(d.times_to_sit_still);
    };

    Dog out_dog;
    msg.unpack(out_dog, dog_unpacker);

    REQUIRE(1 == out_dog.treats.size());
    REQUIRE(5 == out_dog.treats.front());
    REQUIRE(2 == out_dog.times_to_sit_still.size());
}

TEST_CASE("Pack a message into a message") {
    MessagePasser::Message msg_a, msg_b, msg_c;
    msg_a.pack(int(5));

    std::vector<int> some_ints{5, 4, 3, 2};
    msg_b.pack(some_ints);

    msg_a.pack(msg_b);

    int n;
    msg_a.unpack(n);

    REQUIRE(5 == n);

    std::vector<int> vec;
    msg_a.unpack(msg_c);

    msg_c.unpack(vec);

    REQUIRE(some_ints == vec);
}

TEST_CASE("Pack sets like we do vectors and maps") {
    std::set<int> stuff{9, 8, 7, 6, 5};
    MessagePasser::Message msg;
    msg.pack(stuff);

    std::set<int> other_stuff;
    msg.unpack(other_stuff);

    REQUIRE(5 == other_stuff.size());
}

TEST_CASE("Pack a vector of packables into a single message, and unpack them") {
    struct NonTriviallyCopyable {
        std::vector<int> some_ints;
        std::string description;
    };

    std::vector<NonTriviallyCopyable> my_vector;
    my_vector.push_back({{1, 2, 3}, "pepridge"});
    my_vector.push_back({{5, 8, 2}, "farm"});
    my_vector.push_back({{}, "remembers"});

    auto pack = [](MessagePasser::Message& m, const NonTriviallyCopyable& c) {
        m.pack(c.some_ints);
        m.pack(c.description);
    };

    MessagePasser::Message m;
    m.packVector(my_vector, pack);

    auto unpack = [](MessagePasser::Message& m, NonTriviallyCopyable& c) {
        m.unpack(c.some_ints);
        m.unpack(c.description);
    };

    std::vector<NonTriviallyCopyable> my_unpacked_vector;
    m.unpackVector(my_unpacked_vector, unpack);


    REQUIRE(my_unpacked_vector.size() == 3);
    REQUIRE(my_unpacked_vector[0].description == "pepridge");
    REQUIRE(my_unpacked_vector[1].description == "farm");
    REQUIRE(my_unpacked_vector[2].description == "remembers");

    REQUIRE(my_unpacked_vector[0].some_ints == std::vector<int>{1,2,3});
    REQUIRE(my_unpacked_vector[1].some_ints == std::vector<int>{5,8,2});
    REQUIRE(my_unpacked_vector[2].some_ints == std::vector<int>{});
}
