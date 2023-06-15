#pragma once
#include "PluginLoader.h"

namespace inf {
class GeometryAssociationInterface {
  public:
    enum Association { NODE = 0, EDGE = 1, FACE = 2 };

    virtual int associationCount() const = 0;
    virtual Association type(int association_index) const = 0;
    virtual int geometryEntity(int association_index) const = 0;
    virtual int meshNode(int association_index) const = 0;
    virtual void geometryParameter(int association_index, double* parameter) const = 0;
    virtual ~GeometryAssociationInterface() = default;
};
}
