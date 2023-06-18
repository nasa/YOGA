#!/usr/bin/env bash

set -e # exit on first error
set -u # Treat unset variables as error

# Setup bash module environment
. /usr/local/pkgs/modules/init/bash

module purge
module load intel/2019.5.281
module load mpt/2.25

set -x # echo

log=`pwd`/../log-make-check.txt

./bootstrap 2>&1 | tee $log
mkdir -p build
( cd build && \
    export TMPDIR=`pwd` && \
    ../configure \
    --prefix=`pwd` \
    --with-mpi=/opt/hpe/hpc/mpt/mpt-2.25 \
    --with-nanoflann=/ump/fldmd/home/casb-shared/fun3d/fun3d_users/modules/nanoflann/1.3.0 \
    2>&1 | tee $log\
    && make -j distcheck \
            DISTCHECK_CONFIGURE_FLAGS="--with-mpi=/opt/hpe/hpc/mpt/mpt-2.25 --with-nanoflann=/ump/fldmd/home/casb-shared/fun3d/fun3d_users/modules/nanoflann/1.3.0 CXXFLAGS=-O0" 2>&1 | tee $log \
    ) \
    || exit 1
