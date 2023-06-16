#!/bin/bash -e

setup() {
    source test/setup.bats
}


@test "rotate a metric field about a given axis" {
    run inf extensions --load experimental
    run inf extensions --load adaptation
    run yoga extensions --load experimental

    run inf cartmesh --dimensions 10 10 10 -o cart.b8.ugrid
    assert_exist cart.b8.ugrid

    run inf shard -m cart.b8.ugrid -o sharded.b8.ugrid
    assert_exist sharded.b8.ugrid

    run inf metric --mesh sharded.b8.ugrid --implied -o implied.snap
    assert_exist implied.snap

    run yoga rotate-metric --mesh sharded.b8.ugrid --metric implied.snap --axis 0 0 0 0 0 1 --degrees 5 -o rotated.snap
    assert_exist rotated.snap
}

