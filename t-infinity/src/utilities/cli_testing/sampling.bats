#!/bin/bash -e
setup() {
  load 'test/test_helper/bats-support/load'
  load 'test/test_helper/bats-assert/load'
  load 'test/test_helper/bats-file/load'

  run inf cartmesh -d 10 10 10 -o 10x10x10.lb8.ugrid
  run inf distance -m 10x10x10.lb8.ugrid -o distance.snap --tags 1
}

@test "inf sampling a plane " {
  run inf sampling -m 10x10x10.lb8.ugrid --plane 0 0 0.5 0 0 1 -o grid.plt grid.lb8.ugrid
  assert [ -e 'grid.plt' ]
  assert [ -e 'grid.lb8.ugrid' ]
  assert_output -p "Visualizing plane"
}

@test "inf sampling a sphere " {
  run inf sampling -m 10x10x10.lb8.ugrid --sphere 0.5 0.5 0.5 0.5 -o sphere.plt sphere.vtk
  assert [ -e 'sphere.plt' ]
  assert [ -e 'sphere.vtk' ]
  assert_output -p "Visualizing sphere"
}

@test "inf sampling an isosurface " {
  run inf sampling -m 10x10x10.lb8.ugrid --snap distance.snap --iso-surface distance --iso-value 0.25 -o dist-iso.vtk
  echo "output = ${output}"
  assert [ -e 'dist-iso.vtk' ]
}

@test "inf sampling will write snap " {
  run inf sampling -m 10x10x10.lb8.ugrid --snap distance.snap --sphere 0.5 0.5 0.5 0.5 -o sphere.snap
  assert [ -e 'sphere.snap' ]
  run inf snap --snap sphere.snap --at distance
  assert_output -p "Field count: 1"
  assert_output -p "distance"
}
