#include <RingAssertions.h>
#include <FUN3DComplexChecker.h>

using namespace YOGA;

TEST_CASE("check if FUN3D is doing complex"){

    SECTION("When not complex, expect FUN3D to set first 3 doubles for a point") {
        auto noncomplex_get_xyz = [](int node_id, double* xyz) {
            xyz[0] = 9.0;
            xyz[1] = 9.0;
            xyz[2] = 9.0;
        };

        bool is_complex = FUN3DComplexChecker::isComplex(noncomplex_get_xyz);
        REQUIRE_FALSE(is_complex);
    }

    SECTION("When complex, expect FUN3D to set 6 doubles for a point") {
        auto complex_get_xyz = [](int node_id, double* xyz) {
            for (int i = 0; i < 6; i++) xyz[i] = 9.0;
        };

        bool is_complex = FUN3DComplexChecker::isComplex(complex_get_xyz);
        REQUIRE(is_complex);
    }

    SECTION("If 3 are set, but not specifically the first 3, crazy-town"){
        auto strided_get_xyz = [](int node_id, double* xyz) {
          xyz[0] = 9.0;
          xyz[2] = 9.0;
          xyz[4] = 9.0;
        };
        REQUIRE_THROWS(FUN3DComplexChecker::isComplex(strided_get_xyz));
    }

    SECTION("Anything else is nuts and something is wrong") {
        auto bonkers_get_xyz = [](int node_id, double* xyz) {
            for (int i = 0; i < 4; i++) xyz[i] = 9.0;
        };

        REQUIRE_THROWS(FUN3DComplexChecker::isComplex(bonkers_get_xyz));
    }
}