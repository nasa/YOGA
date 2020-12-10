#!/usr/bin/env bash


(cd src/ddata
 ./relink-files.sh || \
  echo "Unable to relink-files in yoga subdirectory src/ddata. One-ring may have not been populated."
)
(cd src/yoga
 ./relink-files.sh || \
  echo "Unable to relink-files in yoga subdirectory src/yoga. One-ring may have not been populated."
)

