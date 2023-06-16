#!/bin/bash
set -e

NPROCS=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.physicalcpu)

time mpirun -np $NPROCS python overset.py overset.json wing_store_bc_info.txt
time mpirun -np $NPROCS python overset-with-adaptation.py overset-with-adaptation.json wing_store_bc_info.txt
