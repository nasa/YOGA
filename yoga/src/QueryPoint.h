#pragma once
#include <parfait/Point.h>

namespace YOGA {
struct QueryPoint {
    Parfait::Point<double> xyz;
    int local_id;
    int component;
};
}
