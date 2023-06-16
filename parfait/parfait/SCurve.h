#pragma once
#include <functional>

namespace Parfait {
namespace SCurve {
    inline std::function<double(double)> create(double min_value = 0.0,
                                                double max_value = 1.0,
                                                double curve_midpoint = 0.0,
                                                double steepness = 1.0) {
        auto curve = [=](double x) {
            x = x - curve_midpoint;
            double k = steepness;
            double a = min_value;
            double b = max_value;
            double alpha = 0.5;
            return a + (b - a) / std::pow(1 + std::exp(-k * x), alpha);
        };
        return curve;
    }
}
}
