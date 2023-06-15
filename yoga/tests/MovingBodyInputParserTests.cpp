#include <RingAssertions.h>
#include "MovingBodyParser.h"

using namespace YOGA;

TEST_CASE("extract body names"){
    std::string input;
    input += "&body_definitions\n";
    input += "output_transform = .false.,\n";
    input += "   n_moving_bodies = 2,            ! number of bodies in motion\n";
    input += "body_name(1) = 'rotor1_blade1', ! name must be in quotes\n";
    input += "n_defining_bndry(1) = 1,        ! number of boundaries that define this body\n";
    input += "defining_bndry(1,1) = 1,        ! index 1: boundry number index 2: body number\n";
    input += "mesh_movement(1) = 'rigid+deform', ! 'rigid', 'deform'\n";
    input += "move_mc(1) = 0,\n\n";

    input += "    body_name(2) = 'rotor1_blade2', ! name must be in quotes\n";
    input += "n_defining_bndry(2) = 1,        ! number of boundaries that define this body\n";
    input += "defining_bndry(1,2) = 3,        ! index 1: boundry number index 2: body number\n";
    input += "mesh_movement(2) = 'rigid+deform', ! 'rigid', 'deform'\n";
    input += "move_mc(2) = 0,";

    auto bodies = MovingBodyInputParser::parse(input);

    REQUIRE(2 == bodies.size());
    REQUIRE("rotor1_blade1" == bodies[0].name);
    REQUIRE(1 == bodies[0].tags.size());
    REQUIRE(1 == bodies[0].tags.front());
    REQUIRE("rotor1_blade2" == bodies[1].name);
    REQUIRE(1 == bodies[1].tags.size());
    REQUIRE(3 == bodies[1].tags.front());
}
