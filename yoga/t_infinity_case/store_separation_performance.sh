#!/bin/bash

source performance_test_functions.sh

MPI_COMMAND=mpirun

NAME=store_sep_150k
SCRIPT=wing_store_composite_script.txt
BCS=wing_store_bc_info.txt
run_sweep "2 4" $NAME $SCRIPT $BCS
