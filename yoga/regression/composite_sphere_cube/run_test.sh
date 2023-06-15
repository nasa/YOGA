#!/bin/bash
set -x
set -u
set -e

echo $(python --version)

NPROCS=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.physicalcpu)

time mpirun -np 2 python regression.py
time mpirun -np 3 python regression.py
time mpirun -np 4 python regression.py
time mpirun -np 5 python regression.py
time mpirun -np $NPROCS python regression.py
