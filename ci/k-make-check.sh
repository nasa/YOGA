#!/usr/bin/env bash

set -e # exit on first error
set -u # Treat unset variables as error

# Setup bash module environment
. /usr/local/pkgs/modules/init/bash

module purge
module load intel_2018.3.222
module load mpt-2.17r14
module load gcc_6.2.0
module load zeromq_4.2.2

set -x # echo

log=`pwd`/../log-make-check.txt

./bootstrap > $log 2>&1
mkdir -p build
( cd build && \
    export TMPDIR=`pwd` && \
    ../configure \
    --prefix=`pwd` \
    --with-mpi=/opt/hpe/hpc/mpt/mpt-2.17r14 \
    --with-zmq=/usr/local/pkgs-modules/zeromq_4.2.2 \
    >> $log 2>&1 \
    && make -j >> $log 2>&1 \
    && make -j distcheck \
            DISTCHECK_CONFIGURE_FLAGS="--with-mpi=/opt/hpe/hpc/mpt/mpt-2.19 --with-zmq=/usr/local/pkgs-modules/zeromq_4.2.2 CXXFLAGS=-O0" >> $log 2>&1 \
    ) \
    || exit 1
