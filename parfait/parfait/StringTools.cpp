#include <algorithm>
#include <cstring>
#include <numeric>
#include <string>
#include "StringTools.h"
#include "RegexMatchers.h"
#include "ToString.h"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <random>
#include <fstream>
#include <regex>
#include <queue>
#include "Throw.h"

namespace Parfait {
namespace StringTools {

    std::vector<std::string> split(const std::string& s, const std::string& delimiter, bool skip_empty) {
        std::vector<std::string> tokens;
        size_t prev = 0, pos = 0;
        do {
            pos = s.find(delimiter, prev);
            if (pos == std::string::npos) pos = s.length();
            std::string token = s.substr(prev, pos - prev);
            if (!token.empty() or not skip_empty) tokens.push_back(token);
            prev = pos + delimiter.length();
        } while (pos < s.length() && prev <= s.length());
        return tokens;
    }
    std::vector<std::string> splitKeepDelimiter(const std::string& s, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t prev = 0, pos = 0;
        do {
            pos = s.find(delimiter, prev);
            if (pos == std::string::npos) pos = s.length();
            std::string token = s.substr(prev, pos - prev);
            if (!token.empty()) tokens.push_back(token);
            if (pos != s.length()) tokens.push_back(delimiter);
            prev = pos + delimiter.length();
        } while (pos < s.length() && prev <= s.length());
        return tokens;
    }
    std::string join(const std::vector<std::string>& pieces, const std::string& delimiter) {
        if (pieces.empty()) return "";
        std::string out;
        for (size_t i = 0; i < pieces.size() - 1; i++) {
            out += pieces[i] + delimiter;
        }
        out += pieces.back();
        return out;
    }
    std::vector<std::string> joinAsVector(const std::vector<std::string>& pieces, const std::string& delimiter) {
        if (pieces.empty()) return {};
        std::vector<std::string> out;
        for (size_t i = 0; i < pieces.size() - 1; i++) {
            out.push_back(pieces[i]);
            out.push_back(delimiter);
        }
        out.push_back(pieces.back());
        return out;
    }

    template <typename T>
    bool contains(const std::vector<T>& container, const T& v) {
        return std::find(container.begin(), container.end(), v) != container.end();
    }

    std::string stripExtension(const std::string& s, const std::vector<std::string>& extensions) {
        auto parts = split(s, ".", false);
        std::vector<std::string> extension_parts;
        for (auto& e : extensions) {
            auto e_parts = split(e, ".");
            extension_parts.insert(extension_parts.end(), e_parts.begin(), e_parts.end());
        }
        std::vector<std::string> output_parts;
        for (auto& p : parts) {
            if (not contains(extension_parts, p)) output_parts.push_back(p);
        }
        return join(output_parts, ".");
    }
    std::string stripExtension(std::string s) {
        size_t lastdot = s.find_last_of(".");
        if (lastdot == std::string::npos) return s;
        return s.substr(0, lastdot);
    }
    std::string stripExtension(std::string s, std::string extension) {
        return stripExtension(s, std::vector<std::string>{std::move(extension)});
    }
    std::string findAndReplace(std::string subject, const std::string& search, const std::string& replace) {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }
    std::string lstrip(std::string s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
        return s;
    }
    std::string rstrip(std::string s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
        return s;
    }
    std::string strip(std::string s) { return lstrip(rstrip(s)); }
    std::string rpad(std::string s, int length, std::string fill) {
        while (int(s.length()) < length) s += fill;
        return s;
    }
    std::string lpad(std::string s, int length, std::string fill) {
        while (int(s.length()) < length) s = fill + s;
        return s;
    }
    std::string tolower(const std::string& s) {
        std::string a = s;
        std::transform(s.begin(), s.end(), a.begin(), ::tolower);
        return a;
    }
    std::string toupper(std::string s) {
        std::string a = s;
        std::transform(s.begin(), s.end(), a.begin(), ::toupper);
        return a;
    }
    bool isDouble(std::string s) {
        s = strip(s);
        if (s.empty()) return false;
        std::string number_regex = RegexMatchers::number();
        std::regex expression(number_regex);
        std::cmatch m;
        bool found = std::regex_search(s.c_str(), m, expression);
        return found and m.position(0) == 0;
    }
    bool isInteger(const std::string& s) {
        if (s.empty()) return false;
        auto begin = s.begin();
        if (s.front() == '-') ++begin;
        return std::find_if(begin, s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
    }
    int toInt(const std::string& s) {
        if (not isInteger(s)) PARFAIT_THROW("Cannot convert non number <" + s + "> to integer");
        std::stringstream buffer(s);
        int value;
        buffer >> value;
        return value;
    }
    long toLong(const std::string& s) {
        if (not isInteger(s)) PARFAIT_THROW("Cannot convert non number <" + s + "> to long");
        std::stringstream buffer(s);
        long value;
        buffer >> value;
        return value;
    }
    std::string getExtension(std::string s) {
        auto pieces = split(s, ".");

        if (pieces.size() > 1)
            return pieces.back();
        else
            return "";
    }
    double toDouble(const std::string& s) {
        try {
            return std::stod(s);
        } catch (std::invalid_argument& e) {
            std::cerr << "invalid argument: " << e.what() << std::endl;
            PARFAIT_THROW("Could not convert <" + s + "> to double");
        }
    }
    std::vector<int> parseIntegerListFromBinaryOperator(std::string s) {
        auto begin_and_end_strings = split(s, ":");
        int begin = toInt(strip(begin_and_end_strings.front()));
        int end = toInt(strip(begin_and_end_strings.back()));
        std::vector<int> out(end - begin + 1);
        for (int i = 0; i < int(out.size()); i++) {
            out[i] = begin + i;
        }
        return out;
    }
    std::vector<std::string> cleanupIntegerListString(std::string s) {
        s = findAndReplace(s, ",", " ");
        s = findAndReplace(s, "\n", " ");
        s = findAndReplace(s, "\t", " ");
        s = findAndReplace(s, "\v", " ");
        s = findAndReplace(s, "\r", " ");
        auto space_split_words = split(s, " ");

        std::vector<std::string> words;
        for (auto w : space_split_words) {
            auto split_words = splitKeepDelimiter(w, ":");
            for (auto sw : split_words) {
                words.push_back(sw);
            }
        }
        return words;
    }

    std::string getNextOrEmpty(std::queue<std::string>& queue) {
        if (queue.empty()) return "";
        return queue.front();
    }
    std::queue<std::string> convertVectorToQueue(const std::vector<std::string>& vec) {
        std::queue<std::string> queue;
        for (auto v : vec) {
            queue.push(v);
        }
        return queue;
    }
    std::vector<int> parseIntegerList(std::string s) {
        auto words = cleanupIntegerListString(s);
        auto queue = convertVectorToQueue(words);
        std::vector<int> out;

        while (not queue.empty()) {
            std::string this_word = queue.front();
            queue.pop();
            std::string next_word = getNextOrEmpty(queue);
            if (next_word == ":") {
                queue.pop();
                auto a = getNextOrEmpty(queue);
                queue.pop();
                if (not isInteger(a)) {
                    PARFAIT_THROW("Error parsing integer list " + s);
                }
                auto list = parseIntegerListFromBinaryOperator(this_word + next_word + a);
                out.insert(out.end(), list.begin(), list.end());
            } else {
                out.push_back(toInt(this_word));
            }
        }

        return out;
    }
    std::vector<std::string> removeWhitespaceOnlyEntries(const std::vector<std::string>& vec) {
        std::vector<std::string> out;
        for (const auto& entry : vec) {
            auto clean = findAndReplace(entry, "\n", "");
            clean = findAndReplace(clean, " ", "");
            if (not clean.empty()) out.push_back(entry);
        }
        return out;
    }
    bool contains(const std::string& s, const std::string& query) { return s.find(query) != std::string::npos; }
    bool startsWith(const std::string& s, const std::string& query) { return s.find(query) == 0; }
    bool endsWith(const std::string& s, const std::string& query) {
        if (s.length() >= query.length()) {
            return (0 == s.compare(s.length() - query.length(), query.length(), query));
        } else {
            return false;
        }
    }
    std::string stripPath(std::string s) {
        size_t found = s.find_last_of("/\\");
        return s.substr(found + 1);
    }
    std::string getPath(std::string s) {
        size_t found = s.find_last_of("/\\");
        return s.substr(0, found + 1);
    }
    std::string removeCommentLines(std::string s, std::string comment_string) {
        auto lines = split(s, "\n");
        std::vector<std::string> out;
        for (const auto& line : lines) {
            if (line.find(comment_string) != 0) out.push_back(line);
        }
        return join(out, "\n");
    }
    std::string randomLetters(int n) {
        std::string s(n, 'a');
        std::random_device rd;
        srand(rd());
        std::transform(s.begin(), s.end(), s.begin(), [](char a) { return a + rand() % 26; });
        return s;
    }
    std::string addExtension(std::string filename, std::string extension) {
        auto e = Parfait::StringTools::getExtension(filename);
        if (e != extension) {
            filename += "." + extension;
        }
        return filename;
    }
    std::vector<std::string> parseQuotedArray(std::string s) {
        std::vector<std::string> output;
        int open = 1;
        int close = 2;

        int search_for = open;

        auto toggle = [&]() {
            if (search_for == open)
                search_for = close;
            else if (search_for == close)
                search_for = open;
        };

        std::string next_phrase;
        for (char c : s) {
            if (c == '"') {
                if (search_for == close) {
                    output.push_back(next_phrase);
                    next_phrase = "";
                }
                toggle();
                continue;
            }
            if (search_for == close) {
                next_phrase.push_back(c);
            }
        }

        return output;
    }
    void parseUgridFilename(std::string& filename,
                            std::string& ugrid_first_extension,
                            std::string& ugrid_last_extension) {
        ugrid_last_extension = getExtension(filename);
        filename = stripExtension(filename);
        ugrid_first_extension = getExtension(filename);
        if (ugrid_first_extension != "b8" and ugrid_first_extension != "lb8")
            ugrid_first_extension = "";
        else
            filename = stripExtension(filename, ugrid_first_extension);
    }
    std::string combineUgridFilename(const std::string& filename,
                                     const std::string& ugrid_first_extension,
                                     const std::string& ugrid_last_extension) {
        std::string s = filename;
        if (not ugrid_first_extension.empty()) {
            s += "." + ugrid_first_extension;
        }
        s += "." + ugrid_last_extension;
        return s;
    }
    std::string stripFromBeginning(std::string s, std::string substring_to_remove) {
        s.erase(0, substring_to_remove.size());
        return s;
    }

    std::vector<std::string> wrapLines(const std::string s, int column_width) {
        std::vector<std::string> lines;

        auto words = split(s, " ");
        for (auto word : words) {
            while (int(word.size()) > column_width) {
                lines.push_back(word.substr(0, column_width - 1) + "-");
                word = word.substr(column_width - 1, word.size());
            }
            if (not lines.empty() and int(lines.back().size() + word.size()) < column_width)
                lines.back() += " " + word;
            else
                lines.push_back(word);
        }
        return lines;
    }
}
}
