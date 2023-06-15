#include "ToString.h"
std::string Parfait::to_string(const std::vector<Parfait::Point<double>>& points) {
    std::string out;
    for (auto& p : points) {
        out += "\n    " + std::to_string(p[0]) + " " + std::to_string(p[1]) + " " + std::to_string(p[2]);
    }
    return out;
}
std::string Parfait::to_string_scientific(double d) {
    std::ostringstream ss;
    ss << d;
    return ss.str();
}
std::string Parfait::to_string(bool b) {
    if (b)
        return "True";
    else
        return "False";
}
std::string Parfait::to_string(const std::unordered_map<std::string, std::string>& some_stuff, std::string delimiter) {
    std::string out;
    for (const auto& pair : some_stuff) out += pair.first + " " + pair.second + delimiter;
    return out;
}
