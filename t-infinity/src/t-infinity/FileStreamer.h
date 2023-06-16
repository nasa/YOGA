#pragma once
#include <memory>
#include <string>
#include <stdio.h>

namespace inf {

class FileStreamer {
  public:
    virtual bool openWrite(std::string filename) = 0;
    virtual bool openRead(std::string filename) = 0;
    virtual void write(const void* data, size_t item_size, size_t num_items) = 0;
    virtual void read(void* data, size_t item_size, size_t num_items) = 0;
    virtual void skip(size_t num_bytes) = 0;
    virtual void close() = 0;
    virtual ~FileStreamer() = default;

    static std::shared_ptr<FileStreamer> create(std::string implementation_name);
};

}