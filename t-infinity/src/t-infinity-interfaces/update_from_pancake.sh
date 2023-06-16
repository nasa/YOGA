#!/usr/bin/env bash

set -e

git clone git@gitlab.larc.nasa.gov:fun3d-developers/pancake.git
cp pancake/include/*.h .
cp pancake/include/*.inc .

(cd pancake && git rev-parse HEAD > ../current_pancake_sha.dat)

rm -rf pancake
