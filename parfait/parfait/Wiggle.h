#pragma once
#include <random>
#include <parfait/Point.h>

namespace Parfait {

class Wiggler {
  public:
    Wiggler() { rng.seed(42); }
    void wiggle(Point<double>& p, double wiggle_distance) { wiggle<3>(p, wiggle_distance); }

    template <int N>
    void wiggle(Point<double, N>& p, double wiggle_distance) {
        Parfait::Point<double, N> w;
        for (size_t i = 0; i < N; i++) {
            w[i] = random(rng);
        }
        w.normalize();
        w *= wiggle_distance;
        p += w;
    }

  private:
    std::mt19937_64 rng;
    std::uniform_real_distribution<double> random{-1.0, 1.0};
};

}