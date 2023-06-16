#pragma once
#include <thread>
namespace YOGA {
inline void short_sleep(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
}
