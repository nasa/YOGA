#! /bin/bash

subpacks=${1:-"build-tools pancake parfait MessagePasser tracer t-infinity"}

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

git submodule update --init --no-fetch -- one-ring

# Sparse Checkout of one-ring

# Find .git directory
git_dir=`git rev-parse --git-dir`

# Check for one-ring submodule
if test -z "${git_dir}/modules/one-ring"
then
  echo "Failed to find one-ring submodule description!"
  exit 1
fi

# This is the subset of files needed to support the distance function
cat > ${git_dir}/modules/one-ring/info/sparse-checkout << EOF
yoga/src
nanoflann/include/nanoflann.hpp
ddata/ddata/*.h*
#t-infinity/src/base/t-infinity/*.h
#t-infinity/src/t-infinity-interfaces/TinfMesh.h
#t-infinity/src/t-infinity-runtime/t-infinity/PluginLoader.h
#t-infinity/src/t-infinity-runtime/t-infinity/Communicator.h
#t-infinity/src/t-infinity-runtime/t-infinity/Mesh.h
#t-infinity/src/t-infinity-runtime/t-infinity/MeshLoader.h
#t-infinity/src/t-infinity-runtime/t-infinity/RepartitionerInterface.h
#t-infinity/src/t-infinity-runtime/t-infinity/VizFactory.h
#t-infinity/src/t-infinity-runtime/t-infinity/VectorFieldAdapter.h
#t-infinity/src/t-infinity-runtime/t-infinity/VizPlugin.h
EOF

(cd one-ring
 git config core.sparseCheckout true
 git checkout --
)
(cd src/ddata
 ./relink-files.sh
)
(cd src/yoga
 ./relink-files.sh
)
