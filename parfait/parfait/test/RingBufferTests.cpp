#include <RingAssertions.h>
#include <parfait/RingBuffer.h>

TEST_CASE("Ring Buffer Exists") {
    int buffer_size = 10;
    Parfait::RingBuffer<int> buffer(buffer_size);
    SECTION("Empty buffer gives empty") {
        auto list = buffer.getContentsInOrder();
        REQUIRE(list.size() == 0);
    }
    SECTION("insert one value, get one") {
        buffer.push(1);
        auto list = buffer.getContentsInOrder();
        REQUIRE(list.size() == 1);
        REQUIRE(list[0] == 1);
    }
    SECTION("insert exactly the buffer size") {
        for (int i = 0; i < buffer_size; i++) {
            buffer.push(i);
        }
        auto list = buffer.getContentsInOrder();
        REQUIRE((int)list.size() == buffer_size);
        for (int i = 0; i < (int)list.size(); i++) {
            REQUIRE(list[i] == i);
        }
    }
    SECTION("insert more than the buffer size") {
        int offset = 5;
        for (int i = 0; i < buffer_size + offset; i++) {
            buffer.push(i);
        }
        auto list = buffer.getContentsInOrder();
        REQUIRE((int)list.size() == buffer_size);
        for (int i = 0; i < (int)list.size(); i++) {
            REQUIRE(list[i] == offset + i);
        }
    }
}
