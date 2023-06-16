#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "list currently loaded extensions" {
    run yoga extensions --load experimental
}
