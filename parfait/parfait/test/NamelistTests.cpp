#include <RingAssertions.h>
#include <parfait/Namelist.h>
#include <parfait/FileTools.h>

using namespace Parfait;

std::string sampleBaselineNamelist() {
    return R"(
        &nonlinear_solver_parameters
          time_accuracy        = '2ndorderOPT'
          time_step_dpsi       = 1.00000
          subiterations        = 30
          temporal_err_control = .true.
          temporal_err_floor   = 0.1
          schedule_cfl         = 1.0  30.0
          schedule_cflturb     = 1.0  10.0
        /
        &tracing
          nranks = 4
          ranks(:) = 0,100,400,800
        /
        &code_run_control
          steps                = 360
          restart_write_freq   = 360
          restart_read         = 'off'
        /
        &flow_initialization
            import_from = 'warm_restart.solb'
        /
    )";
}

TEST_CASE("can get and set different value types") {
    Namelist nml(sampleBaselineNamelist());
    SECTION("string") {
        REQUIRE(nml.has("nonlinear_solver_parameters", "time_accuracy"));
        std::string option = nml.get("nonlinear_solver_parameters", "time_accuracy").asString();
        REQUIRE("2ndorderOPT" == option);
        nml.set("nonlinear_solver_parameters", "time_accuracy", "1storder");
        option = nml.get("nonlinear_solver_parameters", "time_accuracy").asString();
        REQUIRE("1storder" == option);
    }

    SECTION("double") {
        REQUIRE(nml.has("nonlinear_solver_parameters", "time_step_dpsi"));
        double x = nml.get("nonlinear_solver_parameters", "time_step_dpsi").asDouble();
        REQUIRE(1.0 == x);
        nml.set("nonlinear_solver_parameters", "time_step_dpsi", 0.25);
        x = nml.get("nonlinear_solver_parameters", "time_step_dpsi").asDouble();
        REQUIRE(0.25 == x);

        SECTION("multiple") {
            auto v = nml.get("nonlinear_solver_parameters", "schedule_cfl").asDoubles();
            REQUIRE(std::vector<double>{1, 30} == v);
            nml.set("nonlinear_solver_parameters", "schedule_cfl", {1.0, 5.0});
            v = nml.get("nonlinear_solver_parameters", "schedule_cfl").asDoubles();
            REQUIRE(std::vector<double>{1, 5} == v);
        }
    }

    SECTION("bool") {
        REQUIRE(nml.has("nonlinear_solver_parameters", "temporal_err_control"));
        bool b = nml.get("nonlinear_solver_parameters", "temporal_err_control").asBool();
        REQUIRE(b);
        nml.set("nonlinear_solver_parameters", "temporal_err_control", false);
        b = nml.get("nonlinear_solver_parameters", "temporal_err_control").asBool();
        REQUIRE_FALSE(b);
    }

    SECTION("int") {
        REQUIRE(nml.has("nonlinear_solver_parameters", "subiterations"));
        int n = nml.get("nonlinear_solver_parameters", "subiterations").asInt();
        REQUIRE(30 == n);
        nml.set("nonlinear_solver_parameters", "subiterations", 50);
        n = nml.get("nonlinear_solver_parameters", "subiterations").asInt();
        REQUIRE(50 == n);

        SECTION("multiple") {
            REQUIRE(nml.has("tracing", "ranks"));
            auto v = nml.get("tracing", "ranks").asInts();
            REQUIRE(std::vector<int>{0, 100, 400, 800} == v);
            nml.set("tracing", "ranks", {0, 1, 2, 3});
            v = nml.get("tracing", "ranks").asInts();
            REQUIRE(std::vector<int>{0, 1, 2, 3} == v);
        }
    }
}

TEST_CASE("can patch an existing namelist with contents of a second one") {
    Namelist nml(sampleBaselineNamelist());
    REQUIRE(4 == nml.namelists().size());
    REQUIRE(nml.has("tracing"));
    REQUIRE_FALSE(nml.has("global"));
    REQUIRE(nml.has("tracing", "nranks"));
    REQUIRE(4 == nml.get("tracing", "nranks").asInt());
    REQUIRE(nml.has("tracing", "ranks"));
    auto vec = nml.get("tracing", "ranks").asInts();
    REQUIRE(std::vector<int>{0, 100, 400, 800} == vec);

    std::string patch = R"(
        &tracing
          nranks = 3
          ranks(:) = 0,1,7
          secret_option = "bazinga"
        /
    )";

    nml.patch(patch);

    REQUIRE(3 == nml.get("tracing", "nranks").asInt());
    REQUIRE(nml.has("tracing", "ranks"));
    vec = nml.get("tracing", "ranks").asInts();
    REQUIRE(std::vector<int>{0, 1, 7} == vec);
    REQUIRE(nml.has("tracing", "secret_option"));
    REQUIRE("bazinga" == nml.get("tracing", "secret_option").asString());
}

TEST_CASE("patch a namelist that doesn't exist yet") {
    std::string patch = R"(
        &pikachu
            tails = 2
            element = "fire"
        /)";
    Namelist nml(sampleBaselineNamelist());
    nml.patch(patch);
    REQUIRE(nml.has("pikachu"));
    REQUIRE(2 == nml.get("pikachu", "tails").asInt());
    REQUIRE("fire" == nml.get("pikachu", "element").asString());
}

TEST_CASE("can dump to a string") {
    Namelist nml(sampleBaselineNamelist());
    auto s = nml.to_string();

    Namelist roundTrip(s);
    REQUIRE(roundTrip.has("tracing", "ranks"));
    auto ranks = roundTrip.get("tracing", "ranks").asInts();
    REQUIRE(std::vector<int>{0, 100, 400, 800} == ranks);
}