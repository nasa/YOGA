#pragma once
#include <parfait/MotionMatrix.h>
#include <parfait/StringTools.h>
#include <parfait/ToString.h>
#include <parfait/Point.h>
#include <string>
#include <parfait/Lexer.h>
#include <queue>

namespace inf {
class MotionParser {
  public:
    struct GridEntry {
        std::string grid_name;
        std::string mapbc_name;
        std::string domain_name;
        Parfait::MotionMatrix motion_matrix;
        bool has_reflection = false;
        Parfait::Point<double> plane_normal;
        double plane_offset;
    };

    static std::vector<GridEntry> parse(std::string config) {
        auto lexer = getLexer();
        auto token_stream = lexer.extractStrings(lexer.tokenize(config));
        std::queue<std::string> tokens;
        for (auto token : token_stream) tokens.push(token);

        std::vector<GridEntry> entries;
        while (not tokens.empty()) entries.push_back(extractEntry(tokens));
        return entries;
    }

  private:
    static Parfait::Lexer getLexer() {
        using namespace Parfait;
        Lexer::TokenMap token_map;
        token_map.push_back({Lexer::Matchers::whitespace(), ""});
        token_map.push_back({"domain", "DM"});
        token_map.push_back({"grid", "G"});
        token_map.push_back({"mapbc", "BC"});
        token_map.push_back({"translate", "T"});
        token_map.push_back({"rotate", "R"});
        token_map.push_back({"from", "F"});
        token_map.push_back({"to", "TO"});
        token_map.push_back({"reflect", "RF"});
        token_map.push_back({"move", "MV"});
        token_map.push_back({Lexer::Matchers::number(), "R"});
        token_map.push_back({Lexer::Matchers::comment("#"), ""});
        token_map.push_back({Lexer::Matchers::filename(), "R"});
        token_map.push_back({Lexer::Matchers::dashed_word(), "R"});
        token_map.push_back({Lexer::Matchers::c_variable(), "R"});
        Parfait::Lexer lexer(token_map);
        return lexer;
    }

    static bool checkIfDoneWithCurrentEntry(const GridEntry& entry,
                                            const std::queue<std::string>& q) {
        if (q.empty()) return true;
        auto next = q.front();
        if ("G" == next and not entry.grid_name.empty()) return true;
        if ("BC" == next and not entry.mapbc_name.empty()) return true;
        if ("DM" == next and not entry.domain_name.empty()) return true;
        return false;
    }

    static GridEntry extractEntry(std::queue<std::string>& q) {
        GridEntry entry;
        while (not checkIfDoneWithCurrentEntry(entry, q)) {
            if (check(q, "G")) {
                entry.grid_name = q.front();
                q.pop();
                entry.mapbc_name = getMapBcName(q);
            } else if (check(q, "DM")) {
                auto domain_name = extractDomainName(q);
                entry.domain_name = domain_name;
            } else if (check(q, "T")) {
                auto translation = extractPoint(q);
                entry.motion_matrix.addTranslation(translation.data());
            } else if (check(q, "R")) {
                double angle = popDouble(q);
                expect(q, "F");
                auto axis_begin = extractPoint(q);
                expect(q, "TO");
                auto axis_end = extractPoint(q);

                entry.motion_matrix.addRotation(axis_begin.data(), axis_end.data(), angle);
            } else if (check(q, "MV")) {
                std::vector<double> matrix;
                for (int i = 0; i < 16; i++) matrix.push_back(popDouble(q));
                entry.motion_matrix.setMotionMatrix(matrix.data());
            } else if (check(q, "RF")) {
                entry.has_reflection = true;
                auto normal = extractPoint(q);
                entry.plane_normal = normal;
                double offset = popDouble(q);
                entry.plane_offset = offset;
            } else {
                std::string msg = "Unexpected token: " + q.front() + "\n";
                msg += "  valid keywords:\n";
                msg += "    domain\n";
                msg += "    grid\n";
                msg += "    mapbc\n";
                msg += "    rotate\n";
                msg += "    from\n";
                msg += "    to\n";
                msg += "    translate\n";
                msg += "    move\n";
                throw std::logic_error(msg);
            }
        }
        return entry;
    }

    static double popDouble(std::queue<std::string>& tokens) {
        double x = Parfait::StringTools::toDouble(tokens.front());
        tokens.pop();
        return x;
    }

    static std::string extractDomainName(std::queue<std::string>& q) {
        std::string name = "";
        if (not q.empty()) {
            name = q.front();
            q.pop();
        }
        return name;
    }

    static std::array<double, 3> extractPoint(std::queue<std::string>& q) {
        return {popDouble(q), popDouble(q), popDouble(q)};
    }

    static std::string getMapBcName(std::queue<std::string>& tokens) {
        if (not tokens.empty() and check(tokens, "BC")) {
            if (tokens.empty() or not Parfait::StringTools::contains(tokens.front(), "mapbc")) {
                throw std::logic_error("Expected filename after `mapbc`");
            }
            std::string result = tokens.front();
            tokens.pop();
            return result;
        }
        return "";
    }

    static bool check(std::queue<std::string>& tokens, const std::string s) {
        if (not tokens.empty()) {
            if (tokens.front() == s) {
                tokens.pop();
                return true;
            }
        }
        return false;
    }

    static void expect(std::queue<std::string>& tokens, const std::string s) {
        std::string rotation_format_error = "must specify rotate <angle> from <x y z> to <x y z>";
        if (not check(tokens, s)) throw std::logic_error(rotation_format_error);
    }
};
}
