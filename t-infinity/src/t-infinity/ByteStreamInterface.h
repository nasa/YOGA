#pragma once
#include <cstdlib>

namespace inf {

class ByteStreamInterface {
  public:
    virtual size_t size() const = 0;
    virtual void* value() const = 0;
    virtual ~ByteStreamInterface() = default;
};
}
