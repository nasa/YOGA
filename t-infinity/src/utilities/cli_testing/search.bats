#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "list currently loaded extensions" {
    run inf search "exami"
    assert_output -p "examine"
    refute_output -p "snap"
}

