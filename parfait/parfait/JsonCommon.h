#pragma once
#include "Extent.h"
#include "Dictionary.h"
#include "JsonParser.h"
#include "Sphere.h"

namespace Parfait {
namespace Json {
    namespace Common {
        inline Parfait::Extent<double> extent(const Parfait::Dictionary& json) {
            Parfait::Extent<double> e;
            auto lo = json.at("lo").asDoubles();
            auto hi = json.at("hi").asDoubles();
            for (int i = 0; i < 3; i++) {
                e.lo[i] = lo[i];
                e.hi[i] = hi[i];
            }
            return e;
        }

        inline Parfait::Sphere sphere(const Parfait::Dictionary& json) {
            auto center = json.at("center").asDoubles();
            auto radius = json.at("radius").asDouble();
            Parfait::Sphere s({center[0], center[1], center[2]}, radius);
            return s;
        }
    }
}
}
