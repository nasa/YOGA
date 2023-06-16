#pragma once

#include <parfait/RingBuffer.h>

namespace Parfait {
class MovingAverages {
  public:
    inline MovingAverages(int window_width) : ring_buffer(window_width) {}

    inline void push(int v) { ring_buffer.push(v); }

    inline double simpleAverage() const {
        double avg = 0.0;
        auto list = ring_buffer.getContentsInOrder();
        for (auto v : list) {
            avg += v;
        }
        avg /= double(list.size());
        return avg;
    }

    inline double exponentialAverage() const {
        double avg = 0.0;
        auto list = ring_buffer.getContentsInOrder();
        if (list.size() > 0) {
            avg = list[0];
        }
        for (size_t i = 1; i < list.size(); i++) {
            double w = smoothing / double(1 + list.size());
            double v = list[i];
            avg = v * w + (1.0 - w) * avg;
        }
        return avg;
    }

  private:
    Parfait::RingBuffer<double> ring_buffer;
    double smoothing = 2.0;
};
}
