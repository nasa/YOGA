#pragma once
#include <array>

namespace YOGA {
class FUN3DComplexChecker {
  public:
    static bool isComplex(void (*getPoint)(int, double*)) {
        std::array<double, 6> v{Untouched(), Untouched(), Untouched(), Untouched(), Untouched(), Untouched()};
        getPoint(0, v.data());
        if (hasFirst3SetOnly(v)) {
            return false;
        } else if (hasAll6Set(v)) {
            return true;
        } else {
            std::string s = "Can't determine if FUN3D is running complex-mode\n";
            s.append("{");
            for (double x : v)
                if (x == Untouched())
                    s.append("Not-Set, ");
                else
                    s.append("Set, ");
            s.append("}");
            throw std::logic_error(s);
            return false;
        }
    }

  private:
    constexpr static double Untouched() {return std::numeric_limits<double>::max();}

    static bool hasFirst3SetOnly(const std::array<double, 6>& v) {
        bool first_three_set = (0 == std::count(v.begin(), v.begin() + 3, Untouched()));
        bool only_three = (3 == std::count(v.begin(), v.end(), Untouched()));
        return first_three_set and only_three;
    }

    static bool hasAll6Set(const std::array<double, 6>& v) { return 0 == std::count(v.begin(), v.end(), Untouched()); }
};
}