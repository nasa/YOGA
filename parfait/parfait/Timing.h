
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#pragma once

#include <chrono>
#include <iostream>
namespace Parfait {
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::microseconds microseconds;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::seconds seconds;
typedef std::chrono::minutes minutes;
typedef std::chrono::hours hours;

inline auto Now() { return Clock::now(); }

inline double elapsedTimeInSeconds(Clock::time_point t0, Clock::time_point t1);
inline std::string timeDifferenceInMillisecondsAsString(Clock::time_point t0, Clock::time_point t1);
inline std::string readableElapsedTimeAsString(Clock::time_point t0, Clock::time_point t1);

class Stopwatch {
  public:
    inline Stopwatch() { reset(); }

    inline void reset() {
        start = Now();
        running = true;
    }

    inline void stop() {
        _stop = Now();
        running = false;
    }

    inline double seconds() {
        auto end = get_ending_timepoint();
        return elapsedTimeInSeconds(start, end);
    }

    inline std::string to_string() {
        auto end = get_ending_timepoint();
        return readableElapsedTimeAsString(start, end);
    }

  private:
    Clock::time_point start, _stop;
    bool running = true;

    inline Clock::time_point get_ending_timepoint() {
        Clock::time_point end = _stop;
        if (running) {
            end = Now();
        }
        return end;
    }
};

}
#include "Timing.hpp"