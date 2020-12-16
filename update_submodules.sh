#! /bin/bash

subpacks=${1:-"build-tools pancake one-ring parfait MessagePasser tracer t-infinity"}

for i in $subpacks
do
  if test x"none" == x"$subpacks"; then break; fi

  git submodule update --init $i
  if test -x $i/update_submodules.sh
  then
    echo "Updating submodules of $i"
    (cd $i; ./update_submodules.sh none)
  fi
done

./relink-files.sh
