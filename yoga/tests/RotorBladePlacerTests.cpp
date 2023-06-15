#include <RingAssertions.h>
#include <parfait/MotionMatrix.h>
#include <RotorPlacer.h>

using namespace YOGA;
using namespace Parfait;


bool doesRotorBladePositionMatchExpected(const MotionMatrix& expected_motion, const RotorPlacer& rotor_placer, int blade){
    Point<double> some_point {-3,7, 19};
    auto expected_point = some_point;
    auto actual_point = some_point;
    auto actual_motion = rotor_placer.getBladeMotion(blade);
    expected_motion.movePoint(expected_point);
    actual_motion.movePoint(actual_point);
    //printf("Expected: %s Actual: %s\n",expected_point.to_string().c_str(),actual_point.to_string().c_str());
    return expected_point.approxEqual(actual_point);
}

TEST_CASE("place blades using euler angle rotations"){
    Point<double> origin {0,0,0};
    Point<double> x_axis {1,0,0};
    Point<double> y_axis {0,1,0};
    Point<double> z_axis {0,0,1};

    SECTION("Clockwise rotor (default is counter-clockwise") {
        int nblades = 3;
        bool is_clockwise = true;
        RotorPlacer rotor_placer(nblades, is_clockwise);
        MotionMatrix expected_blade0, expected_blade1, expected_blade2;

        expected_blade1.addRotation(origin, z_axis, 120);
        expected_blade2.addRotation(origin, z_axis, 240);

        REQUIRE(doesRotorBladePositionMatchExpected(expected_blade0, rotor_placer, 0));
        REQUIRE(doesRotorBladePositionMatchExpected(expected_blade1, rotor_placer, 1));
        REQUIRE(doesRotorBladePositionMatchExpected(expected_blade2, rotor_placer, 2));
    }


    SECTION("3 blade rotor") {
        int nblades = 3;
        bool is_clockwise = false;
        RotorPlacer rotor_placer(nblades, is_clockwise);
        MotionMatrix expected_blade0, expected_blade1, expected_blade2;

        SECTION("at origin") {
            expected_blade1.addRotation(origin, z_axis, -120);
            expected_blade2.addRotation(origin, z_axis, -240);

            REQUIRE(doesRotorBladePositionMatchExpected(expected_blade0, rotor_placer, 0));
            REQUIRE(doesRotorBladePositionMatchExpected(expected_blade1, rotor_placer, 1));
            REQUIRE(doesRotorBladePositionMatchExpected(expected_blade2, rotor_placer, 2));
        }

        SECTION("translated"){
            Parfait::Point<double> translation {-1,3,2};
            expected_blade0.addTranslation(translation.data());
            expected_blade1.addRotation(origin, z_axis, -120);
            expected_blade1.addTranslation(translation.data());
            expected_blade2.addRotation(origin, z_axis, -240);
            expected_blade2.addTranslation(translation.data());

            rotor_placer.translate(translation);
            REQUIRE(doesRotorBladePositionMatchExpected(expected_blade0, rotor_placer, 0));
            REQUIRE(doesRotorBladePositionMatchExpected(expected_blade1, rotor_placer, 1));
            REQUIRE(doesRotorBladePositionMatchExpected(expected_blade2, rotor_placer, 2));

            SECTION("and rotated about euler angle alpha"){
                double roll_angle = 0.0;
                expected_blade0.addRotation(translation,translation+x_axis, roll_angle);
                expected_blade1.addRotation(translation,translation+x_axis, roll_angle);
                expected_blade2.addRotation(translation,translation+x_axis, roll_angle);

                rotor_placer.roll(roll_angle);

                REQUIRE(doesRotorBladePositionMatchExpected(expected_blade0, rotor_placer, 0));
                REQUIRE(doesRotorBladePositionMatchExpected(expected_blade1, rotor_placer, 1));
                REQUIRE(doesRotorBladePositionMatchExpected(expected_blade2, rotor_placer, 2));
            }

            SECTION("or euler angle beta"){
                double pitching_angle = 0.0;
                expected_blade0.addRotation(translation,translation+y_axis, pitching_angle);
                expected_blade1.addRotation(translation,translation+y_axis, pitching_angle);
                expected_blade2.addRotation(translation,translation+y_axis, pitching_angle);

                rotor_placer.roll(0.0);
                rotor_placer.pitch(pitching_angle);
                rotor_placer.yaw(0.0);

                REQUIRE(doesRotorBladePositionMatchExpected(expected_blade0, rotor_placer, 0));
                REQUIRE(doesRotorBladePositionMatchExpected(expected_blade1, rotor_placer, 1));
                REQUIRE(doesRotorBladePositionMatchExpected(expected_blade2, rotor_placer, 2));
            }

            SECTION("or euler angle gamma"){
                double angle_about_hub = 5.0;
                expected_blade0.addRotation(translation,translation+z_axis, angle_about_hub);
                expected_blade1.addRotation(translation,translation+z_axis, angle_about_hub);
                expected_blade2.addRotation(translation,translation+z_axis, angle_about_hub);

                rotor_placer.roll(0.0);
                rotor_placer.pitch(0.0);
                rotor_placer.yaw(angle_about_hub);

                REQUIRE(doesRotorBladePositionMatchExpected(expected_blade0, rotor_placer, 0));
                REQUIRE(doesRotorBladePositionMatchExpected(expected_blade1, rotor_placer, 1));
                REQUIRE(doesRotorBladePositionMatchExpected(expected_blade2, rotor_placer, 2));
            }
        }

    }

}