#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "merge snap files" {
    run inf extensions --load experimental

    run inf mms --mesh grids/meshb/cube-01.meshb --function 1 -o function_1.snap
    assert_exist function_1.snap

    run inf mms --mesh grids/meshb/cube-01.meshb --function 2 -o function_2.snap
    assert_exist function_2.snap

    # all fields in any snap files will be merged
    run inf snap --snap function_1.snap function_2.snap -o combined.snap
    assert_exist combined.snap

    # if you don't specify -o then the short summary will be printed
    run inf snap --snap combined.snap
    assert_output --partial "function_1"
    assert_output --partial "function_2"

    # rename will rename a field
    run inf snap --snap combined.snap --rename function_1 my_dog_rocks -o dog.snap
    assert_exist dog.snap

    run inf snap --snap dog.snap
    assert_output --partial "my_dog_rocks"

    # rename will rename a field
    run inf snap --snap combined.snap --delete function_1 -o dog.snap
    assert_exist dog.snap
    assert_output --partial "function_1 -> DELETE"
}
