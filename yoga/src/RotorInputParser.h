#pragma once
#include <vector>
#include <string>
#include <parfait/StringTools.h>
#include <parfait/Point.h>

namespace YOGA {
class RotorInputParser {
  public:
    struct RotorConfiguration {
        int blade_count;
        bool clockwise;
        Parfait::Point<double> translation;
        double roll,pitch,yaw;
    };

    static std::vector<RotorConfiguration> parse(const std::string& input) {
        std::vector<RotorConfiguration> rotors;
        auto lines = Parfait::StringTools::split(input, "\n");
        int n_rotors = extractRotorCount(lines);
        for (int i = 0; i < n_rotors; i++) rotors.emplace_back(extractRotor(i, lines));
        return rotors;
    }

  private:
    static int extractRotorCount(const std::vector<std::string>& lines) {
        using namespace Parfait::StringTools;
        auto line = lines[1];
        auto words = split(line, " ");
        auto first_word = words.front();
        return toInt(first_word);
    }

    static RotorConfiguration extractRotor(int n, const std::vector<std::string>& lines) {
        using namespace Parfait::StringTools;
        RotorConfiguration rotor;
        int line_number = linesInHeader() + n * linesPerRotor() + linesBeforeBladeCount();
        auto line = lines[line_number];
        auto words = split(line, " ");
        auto first_word = words.front();
        rotor.blade_count = toInt(first_word);
        rotor.clockwise = extractRotorDirection(n,lines);
        rotor.translation = extractRotorPosition(n, lines);
        auto phi = extractPhi123(n,lines);
        rotor.roll = phi[0];
        rotor.pitch = phi[1];
        rotor.yaw = phi[2];
        return rotor;
    }

    static bool extractRotorDirection(int n, const std::vector<std::string>& lines){
        using namespace Parfait::StringTools;
        int line_number = linesInHeader() + n*linesPerRotor() + linesBeforeRotorDirection();
        auto line = lines[line_number];
        auto words = split(line," ");
        int rotor_direction = toInt(words[5]);
        if(rotor_direction == 1)
            return true;
        return false;
    }

    static Parfait::Point<double> extractRotorPosition(int n, const std::vector<std::string>& lines) {
        using namespace Parfait::StringTools;
        int line_number = linesInHeader() + n * linesPerRotor() + linesBeforeRotorPosition();
        auto line = lines[line_number];
        auto words = split(line, " ");
        Parfait::Point<double> translation;
        for (int i = 0; i < 3; i++) translation[i] = toDouble(words[i]);
        return translation;
    }

    static Parfait::Point<double> extractPhi123(int n, const std::vector<std::string>& lines) {
        using namespace Parfait::StringTools;
        int line_number = linesInHeader() + n * linesPerRotor() + linesBeforePhi123();
        auto line = lines[line_number];
        auto words = split(line, " ");
        Parfait::Point<double> phi_123;
        int skip_xyz = 3;
        for (int i = 0; i < 3; i++) phi_123[i] = toDouble(words[i+skip_xyz]);
        return phi_123;
    }

    static int linesInHeader() { return 2; }
    static int linesPerRotor() { return 23; }
    static int linesBeforeBladeCount() { return 8; }
    static int linesBeforeRotorDirection() { return linesBeforeRotorPosition()+2;}
    static int linesBeforeRotorPosition() { return 4; }
    static int linesBeforePhi123() { return linesBeforeRotorPosition(); }
};
}