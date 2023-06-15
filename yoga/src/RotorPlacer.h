#pragma once
#include <parfait/MotionMatrix.h>

namespace YOGA {
class RotorPlacer {
  public:
    // RotorPlacer uses roll/pitch/yaw based on the nose
    // of the fuselage pointing along -y (like fun3d orientation)
    // FUN3D also expects blades to be numbered based on direction of rotation,
    // e.g., in a counter-clockwise rotor, the second blade will be at -X degrees,
    // but for a clockwise rotor, it would be +X degrees.

    RotorPlacer(int blade_count, bool is_clockwise)
        : n(blade_count),
          sign(is_clockwise?1:-1) {}
    Parfait::MotionMatrix getBladeMotion(int blade_index) const {
        double degrees_per_blade = 360.0 / double(n);
        auto m = rotor_disk_motion;
        double angle = sign * degrees_per_blade * blade_index;
        m.addRotation(rotor_origin, rotor_origin + w, angle);
        return m;
    }

    void translate(const Parfait::Point<double>& dx) {
        rotor_disk_motion.addTranslation(dx.data());
        Parfait::MotionMatrix m(dx.data());
        m.movePoint(rotor_origin);
    }

    void roll(double angle) {
        Parfait::MotionMatrix m;
        Parfait::Point<double> origin{0, 0, 0};
        m.addRotation(origin, u, angle);
        rotor_disk_motion.addRotation(rotor_origin, rotor_origin + u, angle);
        m.movePoint(v);
        m.movePoint(w);
    }

    void pitch(double angle) {
        Parfait::MotionMatrix m;
        Parfait::Point<double> origin{0, 0, 0};
        m.addRotation(origin, v, angle);
        rotor_disk_motion.addRotation(rotor_origin, rotor_origin + v, angle);
        m.movePoint(u);
        m.movePoint(w);
    }

    void yaw(double angle) {
        Parfait::MotionMatrix m;
        Parfait::Point<double> origin{0, 0, 0};
        m.addRotation(origin, w, angle);
        rotor_disk_motion.addRotation(rotor_origin, rotor_origin + w, angle);
        m.movePoint(u);
        m.movePoint(v);
    }

  private:
    const int n;
    const double sign;
    Parfait::Point<double> rotor_origin = {0, 0, 0};
    Parfait::MotionMatrix rotor_disk_motion;
    Parfait::Point<double> u{1, 0, 0};
    Parfait::Point<double> v{0, 1, 0};
    Parfait::Point<double> w{0, 0, 1};
};
}