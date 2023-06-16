#pragma once
#include <parfait/Dictionary.h>
#include <parfait/StringTools.h>

namespace TraceExtractor{
typedef const Parfait::Dictionary& Event;

int extractRank(Event e){
    return e.at("pid").asInt();
}

long extractTime(Event e){
    std::string s = e.at("ts").asString();
    return Parfait::StringTools::toInt(s);
}

bool isBegin(Event e){
    return "B" == e.at("ph").asString();
}

bool isEnd(Event e){
    return "E" == e.at("ph").asString();
}

std::string extractName(Event e){
    return e.at("name").asString();
}
}
