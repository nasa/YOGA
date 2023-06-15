#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "list currently loaded extensions" {
    run inf examine -h
    assert_output -p escription
}

