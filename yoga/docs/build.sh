#!/usr/bin/env bash

set -e

pip install -U -r requirements.txt
make html
