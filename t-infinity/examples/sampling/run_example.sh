#!/bin/bash
set -e

mpirun -np 2 python sample.py config.json
