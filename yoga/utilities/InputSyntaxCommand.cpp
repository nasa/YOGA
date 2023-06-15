#include <stdio.h>
#include <t-infinity/SubCommand.h>
#include <t-infinity/CommonAliases.h>

class ShowInputFileSyntaxCommand : public inf::SubCommand {
  public:
    std::string description() const override { return "Show syntax for input files"; }
    Parfait::CommandLineMenu menu() const override {
        Parfait::CommandLineMenu m;
        m.addFlag(inf::Alias::help(), inf::Help::help());
        m.addFlag({"composite"},"show syntax for composite input file");
        m.addFlag({"boundary-conditions"}, "show syntax for boundary condition input file");
        return m;
    }

    void run(Parfait::CommandLineMenu m, MessagePasser mp) override {
        if(m.has("composite")){
            printCompositeSyntax();
        }
        else if(m.has("boundary-conditions")){
            printBoundarySyntax();
        }
        else{
            printf("%s\n", m.to_string().c_str());
        }
    }

  private:
    void printCompositeSyntax(){
        printf("# Comment lines start with #\n");
        printf("# Any number of component grids can be specified.\n");
        printf("# The same grid file can be used more than once.\n");
        printf("# Each component can optionally have translations/rotations.\n");
        printf("# Rotations are specified in degrees about an axis\n\n");

        printf("# Here is an example with two rotor blades and a fuselage:\n\n");

        printf("# grid and mapbc names are required");
        printf("grid rotor.lb8.ugrid\n");
        printf("mapbc rotor.mapbc\n");
        printf("# can optionally give each domain a name, which will rename boundaries\n");
        printf("#      according to FUN3D conventions for moving_body.input\n");
        printf("domain rotor1_blade1\n");
        printf("# can optionally translate/rotate this individual component grid");
        printf("translate 0 0 1.2\n\n");

        printf("grid rotor.lb8.ugrid\n");
        printf("mapbc rotor.mapbc\n");
        printf("# multiple translations/rotations can be chained together for each component grid\n");
        printf("rotate 180 from 0 0 0 to 0 0 1\n");
        printf("translate 0 0 1.2\n\n");

        printf("grid fuselage.lb8.ugrid\n");
        printf("mapbc fuselage.mapbc\n");
        printf("# grids can also be moved by directly setting a 4x4 transformation matrix\n");
        printf("move\n");
        printf("r11 r12 r13 dx\n");
        printf("r21 r22 r23 dy\n");
        printf("r31 r32 r33 dz\n");
        printf("0.0 0.0 0.0 1.0\n");
    }

    void printBoundarySyntax(){
        printf("# Comments begin with #\n");
        printf("# Boundary conditions should be specified in the same order as \n");
        printf("#      the composite script.\n");
        printf("# This file specifies which boundary tags in the mesh\n");
        printf("#      correspond to each boundary type \n");
        printf("#      (solid, interpolation, y-symmetry)\n\n");
        printf("# Tags can be specified as integers and ranges of integers via `:`");

        printf("# Here is an example with two rotors and a fuselage:\n\n");

        printf("domain rotor1 rotor2\n");
        printf("solid 1 2 8 27\n");
        printf("interpolation 97:99\n\n");

        printf("domain fuselage\n");
        printf("solid 1:10 14 17:20 25\n");
        printf("y-symmetry 11\n\n");

        printf("# note: the rotors are grouped together to reduce duplication,\n");
        printf("# because they are from the same grid and have the same tags,\n");
        printf("# but they can also be specified separately.\n");

    }
};

CREATE_INF_SUBCOMMAND(ShowInputFileSyntaxCommand)