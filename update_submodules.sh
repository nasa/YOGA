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

(cd MessagePasser && rm -rf one-ring && ln -sf ../one-ring one-ring)
(cd parfait && rm -rf one-ring && ln -sf ../one-ring one-ring)
(cd tracer && rm -rf one-ring && ln -sf ../one-ring one-ring)
(cd t-infinity && rm -rf one-ring && ln -sf ../one-ring one-ring)

(cd src/ddata
 ./relink-files.sh
)
(cd src/yoga
 ./relink-files.sh
)
