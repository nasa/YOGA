#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "calc implied metric" {
    run inf extensions --load adaptation

    run inf metric --implied --mesh grids/meshb/cube-01.meshb -o implied_metric.snap
    assert_exist implied_metric.snap

    run inf metric-stats --mesh grids/meshb/cube-01.meshb --metric implied_metric.snap
    assert_output --partial "Complexity"
    assert_output --partial "Stretching"
}
