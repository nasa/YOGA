#!/bin/bash -e


setup() {
  load 'test/test_helper/bats-support/load'
  load 'test/test_helper/bats-assert/load'
  load 'test/test_helper/bats-file/load'
}

@test "Mirror a cartesian mesh based on mesh tags" {
  run inf extensions --load experimental
  run inf cartmesh -d 2 2 2 -o test_cartmesh.lb8.ugrid
  run inf mirror --mesh test_cartmesh.lb8.ugrid --tags 1

  assert_exist mirrored.lb8.ugrid

  run inf examine --mesh mirrored.lb8.ugrid
  # should duplicate all hexes
  assert_output --partial "HEXA_8: 16"
  # but should remove internal faces
  assert_output --partial "QUAD_4: 40"
}


