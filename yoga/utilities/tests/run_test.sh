#!/bin/bash
set -e

if [[ -z "${ONE_RING_SOURCE_DIRECTORY}" ]]; then
  ONE_RING_SOURCE_DIRECTORY=$(cd ../../../.. && pwd)
fi

BATS_DIR="${ONE_RING_SOURCE_DIRECTORY}/third-party-libraries/bats/"
TEST_DIR="bats_working_directory.REMOVE_ME"
GRID_DIR="${ONE_RING_SOURCE_DIRECTORY}/grids/"

tests_to_run=$(ls *.bats)
if [[ $# > 0 ]]; then
    tests_to_run=("$@")
fi

echo "Running: ${tests_to_run}"

# Setup test directory including symlinks
# Please don't commit directory symlinks to repo
rm -rf $TEST_DIR
mkdir -p $TEST_DIR
  cd $TEST_DIR
  ln -s ${GRID_DIR} .
  ln -s ${BATS_DIR} test
cd ..

for t in ${tests_to_run}
do
  cd $TEST_DIR
  cp ../$t .
    ./test/bats/bin/bats $t
  cd ..
done
rm -rf $TEST_DIR

