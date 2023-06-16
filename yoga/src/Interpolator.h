#pragma once
#include <functional>

namespace YOGA{

template<typename T>
class Interpolator{
  public:
    virtual void interpolate(int rank,
                             int index,
                             std::function<void(int, double*)> getSolutionAtNode,
                             double* q) const = 0;
    virtual ~Interpolator() = default;
};
}
