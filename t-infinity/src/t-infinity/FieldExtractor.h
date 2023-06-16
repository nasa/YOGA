#pragma once
#include <t-infinity/FieldInterface.h>
#include <memory>

namespace inf {

class FieldExtractor {
  public:
    virtual std::shared_ptr<FieldInterface> extract(const std::string& name) = 0;
    virtual ~FieldExtractor() {}
};

}