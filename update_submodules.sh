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

# Find .git directory
git_dir_path=${PWD}
until test -d ${git_dir_path}/.git -o "${git_dir_path}" == '/'
do
  git_dir_path=`dirname ${git_dir_path}`
done

# Find one-ring module in .git directory
package=`basename $PWD`
tinfinity_dir="`find ${git_dir_path}/.git -name $package -print`"
tinfinity_dir=${tinfinity_dir:-'.git'}
one_ring_dir="$tinfinity_dir/modules/one-ring"

if test -z "$one_ring_dir"
then
  echo "Failed to find one-ring submodule description!"
  exit 1
fi

if ! test -f ${one_ring_dir}/info/sparse-checkout
then
  # This is the subset of files needed to support the distance function
  cat > ${one_ring_dir}/info/sparse-checkout << EOF
yoga/src
nanoflann/include/nanoflann.hpp
ddata/ddata/Ddata.h
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
fi

(cd one-ring
 git config core.sparseCheckout true
 git checkout --
)
