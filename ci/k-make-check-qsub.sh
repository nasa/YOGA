#!/usr/bin/env bash

set -e # exit on first error
set -u # Treat unset variables as error
set -x # echo

uname -n

testname=yoga
queue=${1:-K3a-fun3d}
nprocs=12
walltime=75

BUILDLOG=$PWD/../log-pbs-out.txt

cat << EOF > ${testname}.pbs
#PBS -S /bin/bash
#PBS -l select=1:ncpus=${nprocs}:mpiprocs=${nprocs}
#PBS -l walltime=00:${walltime}:00
#PBS -q ${queue}
#PBS -N ${testname}
#PBS -o ${testname}
#PBS -W umask=027
#PBS -j oe
#PBS -m n

set -o xtrace   # show commands invoked
set -o errexit  # exit on first error
set -o pipefail # ensure exit with pipe

uname -mrn

cd \$PBS_O_WORKDIR

./ci/k-make-check.sh 2>&1 | tee ${BUILDLOG}

EOF

qsub -V -Wblock=true ${testname}.pbs

