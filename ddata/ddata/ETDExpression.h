#pragma once

#if defined(__CUDACC__)
#define DDATA_DEVICE_FUNCTION __host__ __device__
#else
#define DDATA_DEVICE_FUNCTION
#endif

#define ETD_NO_COPY_CONSTRUCTOR(class)       \
    class(const class&) = delete;            \
    class& operator=(const class&) = delete; \
    class(class &&) = default;               \
    class& operator=(class&&) = default;

#define ETD_TYPES_COMPATIBLE(E1, E2) \
    static_assert(std::is_same<typename E1::value_type, typename E2::value_type>::value, "ETD value type mismatch")

namespace Linearize {
template <typename E>
class ETDExpression {
  public:
    DDATA_DEVICE_FUNCTION const E& cast() const { return static_cast<const E&>(*this); }
};
}
