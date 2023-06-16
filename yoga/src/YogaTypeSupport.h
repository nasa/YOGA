#pragma once
#include <ddata/ComplexDifferentiation.h>
#include <ddata/ETD.h>
#include <complex>
#include <parfait/Point.h>

namespace YOGA {
template<size_t N>
using ADType = Linearize::ETD<N>;
}
