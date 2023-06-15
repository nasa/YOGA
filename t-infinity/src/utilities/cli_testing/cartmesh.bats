#!/bin/bash -e


setup() {
  load 'test/test_helper/bats-support/load'
  load 'test/test_helper/bats-assert/load'
  load 'test/test_helper/bats-file/load'
}

@test "cartmesh produces single hex" {
  inf cartmesh -d 1 1 1 -o test_cartmesh
  run inf examine -m test_cartmesh.lb8.ugrid
  assert_output -p "HEXA_8: 1"
}

@test "cartmesh error checks cell dimensions length" {
  run inf cartmesh -d 1 -o test_cartmesh
  assert_output -p "Could not parse all three cell dimensions"
}

