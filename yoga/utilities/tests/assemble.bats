#!/bin/bash -e

setup() {
    source test/setup.bats
}

function createMeshesAndMapbcFiles {
    run inf cartmesh -d 10 10 10 --lo 4 4 4 --hi 5.5 5.5 5.5  -o inner-cube.b8.ugrid
    local bc="inner-cube.mapbc"
    echo 6 > $bc
    echo "1              -1         interp-box" >> $bc
    echo "2              -1         interp-box" >> $bc
    echo "3              -1         interp-box" >> $bc
    echo "4              -1         interp-box" >> $bc
    echo "5              -1         interp-box" >> $bc

    run inf cartmesh -d 20 20 20 --lo 0 0 0 --hi 10 10 10  -o outer-cube.b8.ugrid
    bc="outer-cube.mapbc"
    echo 6 > $bc
    echo "1              5000              farfield-box" >> $bc
    echo "2              5000              farfield-box" >> $bc
    echo "3              5000              farfield-box" >> $bc
    echo "4              5000              farfield-box" >> $bc
    echo "5              5000              farfield-box" >> $bc
    echo "6              5000              farfield-box" >> $bc

}

function createCompositeInput {
    local f="composite.txt"
    echo "grid inner-cube.b8.ugrid" > $f
    echo "mapbc inner-cube.mapbc" >> $f
    echo "grid outer-cube.b8.ugrid" >> $f
    echo "mapbc outer-cube.mapbc" >> $f
}

function createBoundaryConditionsFile {
    local f="bcs.txt"
    echo "domain A" > $f
    echo "interpolation 1:6" >> $f
    echo "domain B" >> $f
}

function createYogaConfigWithGridImportance {
    local f="yoga.config"
    echo "component-grid-importance 1 0" > $f
}


function assembleAndFilterOutput {
    mpirun -np 3 yoga assemble --file composite.txt --bc bcs.txt 2>&1 | grep "Yoga:"
    #mpirun -np 3 yoga assemble --file composite.txt --bc bcs.txt --viz assembly.vtk
}

@test "but if importance levels are set, yoga should have enough information to perform an assembly" {
    createMeshesAndMapbcFiles
    createCompositeInput
    createBoundaryConditionsFile
    createYogaConfigWithGridImportance

    run assembleAndFilterOutput
    assert_output -p "orphan: 0"
    # Number of receptors should not be zero for a valid assembly
    refute_output -p "receptor: 0"

}

