#include <RingAssertions.h>
#include <RotorInputParser.h>


using namespace YOGA;

std::string sampleInputFile(){
    std::string input =
"  # Rotors  Vinf_ratio  Write Soln   Force Ref Momment Ref\n"
"         4        0.151        1500         1.0        1.0\n"
"=== Front Right Rotor ===================================================\n"
"Rotor Type   Load Type    # Radial    # Normal  Tip Weight\n"
"         1           1          50         180         0.0\n"
"  X0_rotor    Y0_rotor    Z0_rotor        phi1        phi2        phi3\n"
"    0.000        3.000       0.796        0.00        0.00        0.00\n"
"  Vt_ratio  ThrustCoff   PowerCoff        psi0  PitchHinge      DirRot\n"
"       1.0     0.00457       -1.00        0.00         0.0         0\n"
"  # Blades   TipRadius  RootRadius  BladeChord   FlapHinge    LagHinge\n"
"         3       2.000      0.2400       0.121         0.0         0.0\n"
" LiftSlope  alpha, L=0         cd0         cd1         cd2\n"
"      6.28        0.00       0.002        0.00        0.00\n"
"    CL_max      CL_min      CD_max      CD_min       Swirl\n"
"      1.50       -1.50        1.50       -1.50           0\n"
"    Theta0  ThetaTwist     Theta1s     Theta1c  Pitch-Flap\n"
"      0.00        0.00        0.00        0.00        0.00\n"
" # FlapHar       Beta0      Beta1s      Beta1c\n"
"         0        0.00       0.000       0.000\n"
"    Beta2s      Beta2c      Beta3s      Beta3c\n"
"      0.00        0.00        0.00        0.00\n"
"  # LagHar      Delta0     Delta1s     Delta1c\n"
"         0        0.00        0.00        0.00\n"
"   Delta2s     Delta2c     Delta3s     Delta3c\n"
"      0.00        0.00        0.00        0.00\n"
"=== Front Left Rotor ====================================================\n"
"Rotor Type   Load Type    # Radial    # Normal  Tip Weight\n"
"         1           1          50         180         0.0\n"
"  X0_rotor    Y0_rotor    Z0_rotor        phi1        phi2        phi3\n"
"    0.000       -3.000       0.796        -180.00        0.00        0.00\n"
"  Vt_ratio  ThrustCoff   PowerCoff        psi0  PitchHinge      DirRot\n"
"       1.0     0.00457       -1.00        0.00         0.0         0\n"
"  # Blades   TipRadius  RootRadius  BladeChord   FlapHinge    LagHinge\n"
"         3       2.000      0.2400       0.121         0.0         0.0\n"
" LiftSlope  alpha, L=0         cd0         cd1         cd2\n"
"      6.28        0.00       0.002        0.00        0.00\n"
"    CL_max      CL_min      CD_max      CD_min       Swirl\n"
"      1.50       -1.50        1.50       -1.50           0\n"
"    Theta0  ThetaTwist     Theta1s     Theta1c  Pitch-Flap\n"
"      0.00        0.00        0.00        0.00        0.00\n"
" # FlapHar       Beta0      Beta1s      Beta1c\n"
"         0        0.00       0.000       0.000\n"
"    Beta2s      Beta2c      Beta3s      Beta3c\n"
"      0.00        0.00        0.00        0.00\n"
"  # LagHar      Delta0     Delta1s     Delta1c\n"
"         0        0.00        0.00        0.00\n"
"   Delta2s     Delta2c     Delta3s     Delta3c\n"
"      0.00        0.00        0.00        0.00\n"
"=== Rear Right Rotor =====================================================\n"
"Rotor Type   Load Type    # Radial    # Normal  Tip Weight\n"
"         1           1          50         180         0.0\n"
"  X0_rotor    Y0_rotor    Z0_rotor        phi1        phi2        phi3\n"
"    6.000        3.000      1.7500        -180.00        0.00        0.00\n"
"  Vt_ratio  ThrustCoff   PowerCoff        psi0  PitchHinge      DirRot\n"
"       1.0     0.00457       -1.00        0.00         0.0         1\n"
"  # Blades   TipRadius  RootRadius  BladeChord   FlapHinge    LagHinge\n"
"         3       2.000      0.2400       0.121         0.0         0.0\n"
" LiftSlope  alpha, L=0         cd0         cd1         cd2\n"
"      6.28        0.00       0.002        0.00        0.00\n"
"    CL_max      CL_min      CD_max      CD_min       Swirl\n"
"      1.50       -1.50        1.50       -1.50           0\n"
"    Theta0  ThetaTwist     Theta1s     Theta1c  Pitch-Flap\n"
"      0.00        0.00        0.00        0.00        0.00\n"
" # FlapHar       Beta0      Beta1s      Beta1c\n"
"         0        0.00       0.000       0.000\n"
"    Beta2s      Beta2c      Beta3s      Beta3c\n"
"      0.00        0.00        0.00        0.00\n"
"  # LagHar      Delta0     Delta1s     Delta1c\n"
"         0        0.00        0.00        0.00\n"
"   Delta2s     Delta2c     Delta3s     Delta3c\n"
"      0.00        0.00        0.00        0.00\n"
"=== Real Right Rotor ====================================================\n"
"Rotor Type   Load Type    # Radial    # Normal  Tip Weight\n"
"         1           1          50         180         0.0\n"
"  X0_rotor    Y0_rotor    Z0_rotor        phi1        phi2        phi3\n"
"    6.000       -3.000      1.7500        1.00        2.00        3.00\n"
"  Vt_ratio  ThrustCoff   PowerCoff        psi0  PitchHinge      DirRot\n"
"       1.0     0.00457       -1.00        0.00         0.0         1\n"
"  # Blades   TipRadius  RootRadius  BladeChord   FlapHinge    LagHinge\n"
"         3       2.000      0.2400       0.121         0.0         0.0\n"
" LiftSlope  alpha, L=0         cd0         cd1         cd2\n"
"      6.28        0.00       0.002        0.00        0.00\n"
"    CL_max      CL_min      CD_max      CD_min       Swirl\n"
"      1.50       -1.50        1.50       -1.50           0\n"
"    Theta0  ThetaTwist     Theta1s     Theta1c  Pitch-Flap\n"
"      0.00        0.00        0.00        0.00        0.00\n"
" # FlapHar       Beta0      Beta1s      Beta1c\n"
"         0        0.00       0.000       0.000\n"
"    Beta2s      Beta2c      Beta3s      Beta3c\n"
"      0.00        0.00        0.00        0.00\n"
"  # LagHar      Delta0     Delta1s     Delta1c\n"
"         0        0.00        0.00        0.00\n"
"   Delta2s     Delta2c     Delta3s     Delta3c\n"
"      0.00        0.00        0.00        0.00\n";
    return input;
}

TEST_CASE("count rotors"){
    auto input = sampleInputFile();
    auto rotors = RotorInputParser::parse(input);
    REQUIRE(4 == rotors.size());

    REQUIRE_FALSE(rotors[0].clockwise);
    REQUIRE_FALSE(rotors[1].clockwise);
    REQUIRE(rotors[2].clockwise);
    REQUIRE(rotors[3].clockwise);

    for(auto& rotor:rotors){
        REQUIRE(3 == rotor.blade_count);
    }

    REQUIRE(1.0 == rotors[3].roll);
    REQUIRE(2.0 == rotors[3].pitch);
    REQUIRE(3.0 == rotors[3].yaw);

    SECTION("Check rotor 1") {
        Parfait::Point<double> rotor_position {0,3,.796};
        REQUIRE(rotor_position.approxEqual(rotors[0].translation));
    }
}
