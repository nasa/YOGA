#!/usr/bin/env bash
set -e

top_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && cd .. && pwd )"

type python

export ONE_RING_ROOT=${top_dir}
if [[ $# -gt 0 ]]; then
  TINF_INSTALL_PATH=$1
else
  TINF_INSTALL_PATH=${top_dir}/install
fi

export TINF_GRIDS_PATH=${top_dir}/grids
export PYTHONPATH=${TINF_INSTALL_PATH}/python

cd src/python_unit_tests
python -m unittest infinity_plugins_tests.py
#for unittest in *_tests.py; do
#  python -m unittest ${unittest%%.py}
#done
