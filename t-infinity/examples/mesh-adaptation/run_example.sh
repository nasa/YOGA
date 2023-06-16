#!/bin/bash
set -e
set -x

NPROCS=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.physicalcpu)

mpirun -np 2 python interpolation_error.py
mpirun -np $NPROCS python euler_om6_multiscale.py euler_om6_multiscale.json
mpirun -np $NPROCS python euler_om6_opt_goal.py euler_om6_opt_goal.json

