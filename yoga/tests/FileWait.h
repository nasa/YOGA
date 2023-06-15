#pragma once
#include <thread>
#include <parfait/FileTools.h>

inline void waitForFileToExist(const std::string& filename, int max_seconds){
    double slept_for = 0.0;
    while(not Parfait::FileTools::doesFileExist(filename) and slept_for < max_seconds) {
        slept_for += 0.1;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
