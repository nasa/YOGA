#ifndef SYMMETRY_PLANE_H
#define SYMMETRY_PLANE_H
#include <stdio.h>
namespace YOGA {
class SymmetryPlane {
  public:
    enum { X, Y, Z };
    SymmetryPlane() = delete;
    SymmetryPlane(int componentId, int plane, double position);

    int componentId() { return component_id; }
    int getPlane() { return plane; }
    double getPosition() { return position; }

    template <typename PointType>
    void reflect(PointType& p);

    void print() { printf("component id %i plane %i position %lf\n", component_id, plane, position); }

  private:
    int component_id;
    int plane;
    double position;
};

template <typename PointType>
void SymmetryPlane::reflect(PointType& p) {
    double d = 2.0 * (p[plane] - position);
    p[plane] -= d;
}

inline SymmetryPlane::SymmetryPlane(int c, int p, double pos) : component_id(c), plane(p), position(pos) {}
}
#endif
