#!/bin/bash

set -e
set -u
set -x

grid=$1

LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/ump/fldmd/home/casb-shared/fun3d/fun3d_users/onering_modules/linux-centos6-x86_64/gcc-6.2.0/mpich-3.2-duvhaci64ee6q7vsb6on37hdbwrnzt6b/lib:/ump/fldmd/home/mpark/one-ring/install/parfait-plugins/lib /ump/fldmd/home/casb-shared/fun3d/fun3d_users/onering_modules/linux-centos6-x86_64/gcc-6.2.0/mpich-3.2-duvhaci64ee6q7vsb6on37hdbwrnzt6b/bin/mpiexec -np 28 python ./forces.py om6-tmr-${grid}.meshb om6-tmr-${grid}.snap | tee output

rm *.trace

mv hypersolve_forces.json om6-tmr-${grid}-dim-force.json

~/one-ring/hypersolve/utils/compute_hs_force_coefs.py \
--area_reference 1.15315084119231 \
--angle_of_attack 3.06 \
--forces_file om6-tmr-${grid}-dim-force.json \
> om6-tmr-${grid}-force-coef.txt

