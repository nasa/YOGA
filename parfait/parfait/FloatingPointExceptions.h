#pragma once

#include <fenv.h>

#if defined(__APPLE__) && defined(__MACH__) && !defined(__arm64__)

// Public domain polyfill for feenableexcept on OS X
// http://www-personal.umich.edu/~williams/archive/computation/fe-handling-example.c

inline int feenableexcept(unsigned int excepts) {
    static fenv_t fenv;
    unsigned int new_excepts = excepts & FE_ALL_EXCEPT;
    // previous masks
    unsigned int old_excepts;

    if (fegetenv(&fenv)) {
        return -1;
    }
    old_excepts = fenv.__control & FE_ALL_EXCEPT;

    // unmask
    fenv.__control &= ~new_excepts;
    fenv.__mxcsr &= ~(new_excepts << 7);

    return fesetenv(&fenv) ? -1 : old_excepts;
}

inline int fedisableexcept(unsigned int excepts) {
    static fenv_t fenv;
    unsigned int new_excepts = excepts & FE_ALL_EXCEPT;
    // all previous masks
    unsigned int old_excepts;

    if (fegetenv(&fenv)) {
        return -1;
    }
    old_excepts = fenv.__control & FE_ALL_EXCEPT;

    // mask
    fenv.__control |= new_excepts;
    fenv.__mxcsr |= new_excepts << 7;

    return fesetenv(&fenv) ? -1 : old_excepts;
}

namespace Parfait {
inline void enableFloatingPointExceptions() { feenableexcept(FE_ALL_EXCEPT); }

inline void disableFloatingPointExceptions() { fedisableexcept(FE_ALL_EXCEPT); }
}

#else

namespace Parfait {
inline void enableFloatingPointExceptions() {}

inline void disableFloatingPointExceptions() {}
}

#endif