#!/bin/bash -e

setup() {
    source test/setup.bats
}

function generateBaselineAndPatches {
    echo "&pikachu" > my_case.nml
    echo "  tails = 2" >> my_case.nml
    echo "/" >> my_case.nml

    echo "&pikachu" > patch1.nml
    echo "  tails = 1" >> patch1.nml
    echo "/" >> patch1.nml

    echo "&charizard" > patch2.nml
    echo "  element = \"fire\"" >> patch2.nml
    echo "/" >> patch2.nml
}

@test "list currently loaded extensions" {
    run generateBaselineAndPatches

    run nml patch --base my_case.nml --patches patch1.nml patch2.nml -o patched.nml

    assert_exist patched.nml

    run cat patched.nml
    assert_output --partial pikachu
    assert_output --partial "= 1"
    assert_output --partial charizard
}

