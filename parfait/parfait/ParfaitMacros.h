#pragma once

#if defined(__CUDACC__)
#define PARFAIT_DEVICE __host__ __device__
#else
#define PARFAIT_DEVICE
#endif

#define PARFAIT_INLINE inline PARFAIT_DEVICE
