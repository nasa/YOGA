#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "list available functions" {
    run inf extensions --load experimental
    run inf mms --list-functions
    assert_output --partial "Available"
    assert_output --partial "1"
    assert_output --partial "sin"
}

@test "generate test fields" {
    run inf extensions --load experimental
    run inf mms --mesh grids/meshb/cube-01.meshb --function 1 -o function_1.snap
    assert_exist function_1.snap

    run inf mms --mesh grids/meshb/cube-01.meshb --function 2 -o function_2.snap
    assert_exist function_2.snap
}

