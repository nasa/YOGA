#!/bin/bash -e


setup() {
  load 'test/test_helper/bats-support/load'
  load 'test/test_helper/bats-assert/load'
  load 'test/test_helper/bats-file/load'
}

@test "inf iextrude maintains node GID order of the original 2D plane" {
  run inf cartmesh -d 5 5 5 -o box.lb8.ugrid
  assert_exist box.lb8.ugrid

  run inf plot -m box.lb8.ugrid --tags 1 -o side.lb8.ugrid
  assert_exist side.lb8.ugrid

  run inf shard -m side.lb8.ugrid -o tri.lb8.ugrid
  assert_exist tri.lb8.ugrid

  run inf plot -m tri.lb8.ugrid -o tri.meshb
  assert_exist tri.meshb

  run inf plot -m tri.lb8.ugrid --gids -o gids_lb8.snap
  assert_exist gids_lb8.snap

  run inf plot -m tri.meshb --gids -o gids_meshb.snap
  assert_exist gids_meshb.snap

  run inf snap-diff -m tri.lb8.ugrid --snap-1 gids_lb8.snap --snap-2 gids_meshb.snap
  assert_output -p "Field node-global-id L2 diff 0.000000e+00"
  assert_output -p "Field cell-global-id L2 diff 0.000000e+00"
}
