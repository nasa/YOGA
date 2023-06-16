#!/bin/bash -e


setup() {
  load 'test/test_helper/bats-support/load'
  load 'test/test_helper/bats-assert/load'
  load 'test/test_helper/bats-file/load'
}

@test "inf extrude can automatically read a meshb" {
  run inf extrude -m grids/meshb/square.meshb
  assert_output -p "Nodes:     184"
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

  run inf iextrude -m tri.meshb -o axi.lb8.ugrid --angle 15
  assert_exist axi.lb8.ugrid

  run inf test-fields -m tri.meshb -o tri.snap
  assert_exist tri.snap

  run inf test-fields -m axi.lb8.ugrid -o axi.snap
  assert_exist axi.snap

  run inf validate -m axi.lb8.ugrid
  assert_output -p "Mesh has passed all mesh checks."

  run inf plot -m tri.meshb --snap axi.snap -o tri-from-axi.snap
  assert_exist tri-from-axi.snap

  run inf snap-diff -m tri.meshb --snap-1 tri.snap --snap-2 tri-from-axi.snap
  assert_output -p "Field test L2 diff 0.000000e+00"

}
