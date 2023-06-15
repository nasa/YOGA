#pragma once
#include <parfait/Point.h>

template<typename T>
inline T linearTestFunction(const Parfait::Point<T>& p) { return 2.3 * p[0] + 9.2 * p[1] + 3.9 * p[2] + 1.2; }