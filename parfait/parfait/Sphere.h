#pragma once
#include "Point.h"
#include "Extent.h"
#include "Facet.h"

namespace Parfait {
class Sphere {
  public:
    Sphere() = default;
    Sphere(const Parfait::Point<double>& c, double r);
    Parfait::Point<double> center;
    double radius;

    bool intersects(const Parfait::Point<double>& p);

    bool intersects(const Parfait::Extent<double>& e);

    std::vector<Parfait::Facet> facetize() const;

    void visualize(std::string filename);
};

Sphere boundingSphere(const std::vector<Parfait::Point<double>>& points);

Sphere boundingSphere(Parfait::Facet f);
}
