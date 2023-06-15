#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "calc implied metric" {
    run inf extensions --load adaptation
    assert_output --partial "adaptation"

    run inf refine-metric-to-snap --mesh grids/lb8.ugrid/om6.lb8.ugrid --file grids/metric_from_refine.solb -o metric.snap
    assert_exist metric.snap
}

