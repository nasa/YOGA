#pragma once
#include <memory>
#include "FieldInterface.h"

namespace inf {
class VizInterface {
  public:
    virtual void addField(std::shared_ptr<FieldInterface> field) = 0;
    virtual void visualize() = 0;
    virtual ~VizInterface() = default;
};
}
