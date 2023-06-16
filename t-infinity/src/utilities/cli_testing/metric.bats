#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "calc implied metric" {
    run inf extensions --load adaptation
    assert_output --partial "adaptation"

    run inf metric --implied --mesh grids/meshb/cube-01.meshb -o implied_metric.snap
    assert_exist implied_metric.snap
}

@test "export metric as pointwise sources and scale to target node count" {
    run inf extensions --load adaptation

    run inf metric --implied --mesh grids/meshb/cube-01.meshb -o implied_metric.pcd
    assert_exist implied_metric.pcd

    run inf metric --implied --mesh grids/meshb/cube-01.meshb -o scaled_metric.pcd --target-node-count 5000
    run diff implied_metric.pcd scaled_metric.pcd
    assert_output --partial "00000"
}

@test "scale a given metric to a target node count" {
    run inf extensions --load adaptation

    run inf metric --implied --mesh grids/meshb/cube-01.meshb -o implied_metric.snap
    run inf metric --metrics implied_metric.snap --mesh grids/meshb/cube-01.meshb -o implied_metric_scaled.pcd --target-node-count 5000
    assert_exist implied_metric_scaled.pcd
}

@test "intersect three metrics (two from scalar fields and one metric)" {
    run inf extensions --load adaptation
    run inf extensions --load experimental

    local mesh=grids/meshb/cube-01.meshb

    run inf mms --mesh ${mesh} --function 1 -o A.snap
    assert_exist A.snap
    run inf mms --mesh ${mesh} --function 2 -o B.snap
    assert_exist B.snap

    run inf snap --snap A.snap B.snap -o combined.snap
    assert_exist combined.snap

    run inf metric --implied --mesh {mesh} -o implied_metric.snap

    run inf metric --mesh ${mesh} --snap combined.snap --metrics implied_metric.snap --intersect -o intersected.snap
    assert_exist intersected.snap
}

@test "prescribe a normal spacing in the metric via mesh tags" {
    run inf extensions --load adaptation

    run inf metric --implied --mesh grids/meshb/cube-01.meshb --normal-spacing 0.1 --complexity 1000 --tags 6 -o normal_spacing_metric.snap
    assert_exist normal_spacing_metric.snap
    assert_output --partial "[10/10] Current complexity: 1.000000e+03 Target Complexity: 1.000000e+03"
}
