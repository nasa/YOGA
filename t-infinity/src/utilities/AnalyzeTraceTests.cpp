#include <RingAssertions.h>
#include <parfait/JsonParser.h>
#include "AnalyzeTraceCommand.h"

TEST_CASE("do a test") {
    std::string s =
        "[    {\n"
        "        \"cat\": \"DEFAULT\",\n"
        "        \"name\": \"Domain Assembly\",\n"
        "        \"ph\": \"B\",\n"
        "        \"pid\": 9,\n"
        "        \"tid\": -1431287744,\n"
        "        \"ts\": \"2\"\n"
        "    },"
        "    {\n"
        "        \"cat\": \"DEFAULT\",\n"
        "        \"name\": \"Domain Assembly\",\n"
        "        \"ph\": \"E\",\n"
        "        \"pid\": 9,\n"
        "        \"tid\": -1431287744,\n"
        "        \"ts\": \"200\"\n"
        "    }"
        "]";

    auto events = Parfait::JsonParser::parse(s).asObjects();
    REQUIRE(2 == events.size());
    auto& e = events.front();
    REQUIRE(9 == TraceExtractor::extractRank(e));
    REQUIRE(2 == TraceExtractor::extractTime(e));
    REQUIRE(TraceExtractor::isBegin(e));
    REQUIRE_FALSE(TraceExtractor::isEnd(e));
    REQUIRE("Domain Assembly" == TraceExtractor::extractName(e));

    e = events[1];
    REQUIRE(200 == TraceExtractor::extractTime(e));
    REQUIRE_FALSE(TraceExtractor::isBegin(e));
    REQUIRE(TraceExtractor::isEnd(e));
}