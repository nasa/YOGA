#!/usr/bin/env bash
set -e

if test "$#" -ne 1; then
    echo "one command line argument: /your/install/prefix"
    exit 1
fi

echo "Installing bindings to: $1"
mkdir -p build
cd build
#cmake .. -DBUILD_TESTS:BOOL=false -DBUILD_RUNTIME:bool=false -DBUILD_EXAMPLES:bool=false -DCMAKE_INSTALL_PREFIX=$1
cmake .. -DBUILD_TESTS:BOOL=false -DBUILD_RUNTIME:bool=false -DBUILD_EXAMPLES:bool=true -DCMAKE_INSTALL_PREFIX=$1
make
make install
