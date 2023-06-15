#pragma once

#include <set>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "Point.h"
#include <unordered_map>

namespace Parfait {
template <typename T>
std::string bigNumberToStringWithCommas(T value) {
    std::string numWithCommas = std::to_string(long(value));
    int insertPosition = numWithCommas.length() - 3;
    while (insertPosition > 0) {
        numWithCommas.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return numWithCommas;
}

template <typename T, typename ToString>
std::string to_string(const std::vector<T>& some_stuff, ToString to_string_small, std::string delimiter) {
    std::string out;
    for (const auto& s : some_stuff) out += to_string_small(s) + delimiter;
    return out;
}

std::string to_string(const std::vector<Parfait::Point<double>>& points);

std::string to_string_scientific(double d);

template <typename T>
std::string to_string(const std::vector<T>& some_stuff, std::string delimiter = " ") {
    std::string out;
    for (const auto& s : some_stuff) out += std::to_string(s) + delimiter;
    return out;
}

template <typename T, unsigned long Dim>
std::string to_string(const std::array<T, Dim>& some_stuff, std::string delimiter = " ") {
    std::string out;
    for (const auto& t : some_stuff) out += std::to_string(t) + delimiter;
    return out;
}

template <typename T>
std::string to_string(const T& t, std::string delimiter = " ") {
    return std::to_string(t);
}

template <>
inline std::string to_string(const std::string& t, std::string delimiter) {
    return t;
}

template <typename T>
std::string to_string(const std::set<T>& some_stuff, std::string delimiter = " ") {
    std::string out;
    for (const auto& s : some_stuff) out += to_string(s) + delimiter;
    return out;
}

template <typename T, typename U>
std::string to_string(const std::map<T, U>& some_stuff, std::string delimiter = " ") {
    std::string out;
    for (const auto& pair : some_stuff)
        out += std::to_string(pair.first) + " " + std::to_string(pair.second) + delimiter;
    return out;
}

template <typename T>
std::string to_string(const std::map<T, std::string>& some_stuff, std::string delimiter = " ") {
    std::string out;
    for (const auto& pair : some_stuff) out += std::to_string(pair.first) + " " + pair.second + delimiter;
    return out;
}

std::string to_string(const std::unordered_map<std::string, std::string>& some_stuff, std::string delimiter = " ");

template <typename T, typename U>
std::string to_string(const std::unordered_map<T, U>& some_stuff, std::string delimiter = " ") {
    std::string out;
    for (const auto& pair : some_stuff)
        out += std::to_string(pair.first) + " " + std::to_string(pair.second) + delimiter;
    return out;
}

template <>
inline std::string to_string(const std::vector<std::string>& some_stuff, std::string delimiter) {
    std::string out;
    for (const auto& s : some_stuff) out += s + delimiter;
    return out;
}

std::string to_string(bool b);

}
