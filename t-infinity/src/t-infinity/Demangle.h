#pragma once
#include <string>
#include <vector>

namespace inf {
std::string demangle(const std::string& s);
std::vector<std::string> breakSymbolLineIntoWords(const std::string& line);
}
