#pragma once
#include <memory>

#ifdef ALWAYS_MANGLE
#define mangle(f) infinity_LTX_##f
#elif defined(__PIC__) || defined(__pic__) || defined(PIC)
#define mangle(f) f
#else
#define mangle(f) infinity_LTX_##f
#endif

template <typename T>
class SharedPointerWrapper {
  public:
    SharedPointerWrapper(std::shared_ptr<T> ptr) : shared_ptr(ptr) {}
    std::shared_ptr<T> shared_ptr;
};
