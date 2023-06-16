#!/bin/bash -e
setup() {
    source test/setup.bats
}

@test "inf plot can create a ugrid and snap file" {
  run inf cartmesh -d 10 10 10 -o test.lb8.ugrid
  run inf distance -m test.lb8.ugrid -o distance.snap --tags 1
  run inf plot -m test.lb8.ugrid --snap distance.snap -o sphere.lb8.ugrid
  assert_exist 'sphere.snap'
  assert_exist 'sphere.lb8.ugrid'
}

@test "inf plot can convert snap to solb (and back)" {
  run inf cartmesh -d 10 10 10 -o test.lb8.ugrid
  run inf distance -m test.lb8.ugrid -o distance.snap --tags 1
  run inf plot -m test.lb8.ugrid --snap distance.snap -o distance.solb
  assert_exist 'distance.solb'
  assert_exist 'distance.solb.names'
  run cat distance.solb.names
  assert_output "distance"

  run inf plot -m test.lb8.ugrid --snap distance.solb -o back.snap
  assert_exist "back.snap"
  run inf snap -s back.snap 
  assert_output --partial "distance"
}

@test "inf plot can convert use --select with solb" {
  run inf cartmesh -d 10 10 10 -o test.lb8.ugrid
  run inf plot -m test.lb8.ugrid --distance 1 --cell-quality -o distance.solb 
  assert_exist 'distance.solb'
  assert_exist 'distance.solb.names'
  run cat distance.solb.names
  assert_output --partial "plot.distance"
  assert_output --partial "Hilbert_cost"

  run inf plot -m test.lb8.ugrid --snap distance.solb --select plot.distance -o dog.solb
  assert_exist "dog.solb"
  assert_exist "dog.solb.names"
  run cat dog.solb.names
  refute_output --partial "Hilbert_cost"
}

