#!/bin/bash
set -x
set -u
set -e

if [[ $# -ne 1 ]]; then
  GRIDS_DIR=$(cd ../../../grids && pwd)
else
  GRIDS_DIR=$1
fi

ln -sf ${GRIDS_DIR}/gmsh/mixed_element_cube.msh .
ln -sf ${GRIDS_DIR}/gmsh/inner_sphere.msh .

NPROCS=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.physicalcpu)

function run_stationary() {
    local np=$1
    if [[ $np -le $NPROCS ]]; then
        time mpirun -np $np yoga assemble --file composite.txt --bc boundary_conditions.txt --debug
        mpirun -np 1 yoga verify --file yoga_debug.json --golden golden.json
    fi
}

run_stationary 2
run_stationary 3
run_stationary 4
run_stationary 5
run_stationary $NPROCS


function run_rotating() {
    local np=$1
    if [[ $np -le $NPROCS ]]; then
        time mpirun -np $np yoga fix-orphan --file composite.txt --bc boundary_conditions.txt --steps 360 --degrees 1 --axis 0,0,1 --components 0
    fi

}

#run_rotating 2
