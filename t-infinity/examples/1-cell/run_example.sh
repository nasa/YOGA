#!/bin/bash
set -e

mpirun -np 1 python 1-cell.py hypersolve.json
