#pragma once
#include <vector>

namespace Parfait {
template <typename T>
class RingBuffer {
  public:
    inline RingBuffer(int size) : buffer_size(size) { values.reserve(size); }

    void push(const T& t) {
        if (values.size() < buffer_size) {
            values.push_back(t);
        } else {
            values[next_index] = t;
            next_index = (next_index + 1) % buffer_size;
        }
    }

    std::vector<T> getContentsInOrder() const {
        size_t length = std::min(buffer_size, values.size());
        std::vector<T> output(length);
        for (size_t i = 0; i < length; i++) {
            size_t index = (next_index + i) % buffer_size;
            output[i] = values[index];
        }
        return output;
    }

  private:
    size_t buffer_size;
    std::vector<T> values;
    int next_index = 0;
};
}
