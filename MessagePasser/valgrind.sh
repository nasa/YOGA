#!/usr/bin/env bash
set -e

build_dir=$1

source ../misc/valgrind_helpers

cd $build_dir/MessagePasser/MessagePasser
mpirun -np 1 valgrind ${DEFAULT_ARGS} --gen-suppressions=all ./MessagePasserTests
