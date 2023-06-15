#include <RingAssertions.h>
#include <parfait/NamelistParser.h>
#include <parfait/JsonParser.h>

using namespace Parfait;

void test_namelist(std::string namelist, std::string json_string) {
    auto namelist_json_string = NamelistParser::parse(namelist).dump();
    REQUIRE(namelist_json_string == json_string);
}

TEST_CASE("Empty namelist returns empty Dictionary") { REQUIRE("{}" == NamelistParser::parse("").dump()); }

TEST_CASE("Empty namelists work with lowercase names") {
    test_namelist("&GROUP \n /", "[{\"group\":{}}]");
    test_namelist("$GROUP \n $", "[{\"group\":{}}]");
}

TEST_CASE("Multiple namelists work") {
    test_namelist("&GROUP \n /\n&ANOTHER/", "[{\"group\":{}},{\"another\":{}}]");
    test_namelist("&same_name \n /\n&same_name/", "[{\"same_name\":{}},{\"same_name\":{}}]");
}

TEST_CASE("Malformed namelists don't work") {
    REQUIRE_THROWS(NamelistParser::parse("&GROUP \n $"));
    REQUIRE_THROWS(NamelistParser::parse("$GROUP \n /"));
    REQUIRE_THROWS(NamelistParser::parse("garbage"));
    REQUIRE_THROWS(NamelistParser::parse("&GROUP / garbage"));
    REQUIRE_THROWS(NamelistParser::parse("&GROUP array=.3*6 /"));
    REQUIRE_THROWS(NamelistParser::parse("&GROUP array=&test /"));
    REQUIRE_THROWS(NamelistParser::parse("&GROUP undeliminated_string=test string /"));
    REQUIRE_THROWS(NamelistParser::parse("&GROUP a='test','test'/"));  // can't have an array of strings
    REQUIRE_THROWS(NamelistParser::parse("&GROUP a=1,.t./"));          // can't have an array of mixed types
}

TEST_CASE("Can put stuff in namelists") {
    test_namelist("&GROUP INT=1 FLOAT=1.1 STRING=\"test\"/",
                  "[{\"group\":{\"float\":1.1,\"int\":1,\"string\":\"test\"}}]");  // note keys get alphabetized
    test_namelist("&GROUP string1=\"1,1\", string2='true' /",
                  "[{\"group\":{\"string1\":\"1,1\",\"string2\":\"true\"}}]");
    test_namelist("&GROUP b1=.t., b2=.true., b3=.TrUe./", "[{\"group\":{\"b1\":true,\"b2\":true,\"b3\":true}}]");
    test_namelist("&GROUP b1=.F., b2=.false., b3=.fAlsE./", "[{\"group\":{\"b1\":false,\"b2\":false,\"b3\":false}}]");
}

TEST_CASE("Arrays work in namelists") {
    test_namelist("&GROUP A1=1,1, a2=10*3, a3=.1,2*.3,5/",
                  "[{\"group\":{\"a1\":[1,1],\"a2\":[3,3,3,3,3,3,3,3,3,3],\"a3\":[0.1,0.3,0.3,5]}}]");
    test_namelist("&GROUP b1=.F.,.t.,.t. b2=2*.true.,.false./",
                  "[{\"group\":{\"b1\":[false,true,true],\"b2\":[true,true,false]}}]");

    // test_namelist("&GROUP a(1)=1/", "[{\"group\":{\"a\":[1]}}]");
}

TEST_CASE("OVERFLOW namelist works") {
    // based loosly off eggers_adapt
    std::string namelist = R"(
 &GLOBAL
    NSTEPS=   0,  RESTRT= .F.,  NSAVE = 0,
    MULTIG= .T.,  FMG   = .T.,  FMGCYC= 1000,1000,
    NQT   = 205,
    DTPHYS= 0.5,
    /

 &OMIGLB
    IRUN  = 0,  DYNMCS= .F.,  I6DOF = 2,
    /
 &BRKINP /
 &GROUPS /

 &FLOINP
    REFMACH= 1.0,  FSMACH= 0.01,  REY   = 0.586E6,  TINF  = 525,
    /

 &GRDNAM
    NAME  = 'nozzle',
    /
 &BCINP
    IBTYP =   5, 16,141, 22,
    IBDIR =  -2,  2,  1,  3,
!    BCPAR1(3)= 11.03195,
!    BCPAR2(2)= 1.,
    /

 &GRDNAM
    NAME  = 'upper',
    /
 )";

    std::string expected_json = R"(
  [ { "global": {
        "nsteps":0, "restrt":false, "nsave":0, "multig":true, "fmg":true, "fmgcyc":[1000,1000], "nqt":205, "dtphys":0.5
      }
    },
    { "omiglb": {
        "irun":0, "dynmcs":false, "i6dof":2
      }
    },
    { "brkinp": {} }, {"groups": {}},
    { "floinp": {
        "refmach":1.0, "fsmach": 0.01, "rey":0.586E6, "tinf":525
      }
    },
    { "grdnam": {"name":"nozzle"} },
    { "bcinp": {
        "ibtyp":[5,16,141,22],
        "ibdir":[-2,2,1,3]
        }
    },
    { "grdnam": {"name":"upper"} }
  ])";

    auto json_dump = JsonParser::parse(expected_json).dump();
    auto nml_dump = NamelistParser::parse(namelist).dump();

    REQUIRE(nml_dump == json_dump);
}

TEST_CASE("can set entire array with colon operator") {
    std::string nml =
        "&pikachu\n"
        "   shock_lvls(:) = 1,2,3,4"
        "/\n"
        "&bulbasaur\n"
        "/\n";
    auto dict = NamelistParser::parse(nml);
    REQUIRE(2 == dict.size());
    REQUIRE(dict[0].has("pikachu"));
    REQUIRE(dict[0]["pikachu"].has("shock_lvls"));
    printf("array: %s\n", dict[0]["pikachu"].dump().c_str());
    REQUIRE(dict[0]["pikachu"]["shock_lvls"].asInts() == std::vector<int>{1, 2, 3, 4});
}