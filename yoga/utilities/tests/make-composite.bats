#!/bin/bash -e

setup() {
    source test/setup.bats
}

function createMapbcFiles {
    local bc="inner.mapbc"
    echo 4 > $bc
    echo "1              -1         inner_overset" >> $bc
    echo "2              4000           aft" >> $bc
    echo "3              4000           fore" >> $bc
    echo "4              4000           fore" >> $bc

    bc="outer.mapbc"
    echo 4 > $bc
    echo "1              -1              outer_overset" >> $bc
    echo "2              -1              outer_overset" >> $bc
    echo "3              -1              outer_overset" >> $bc
    echo "4              -1              outer_overset" >> $bc

    bc="wake.mapbc"
    echo 1 > $bc
    echo "1              -1              wake_overset" >> $bc
}

function createCompositeInputWithDomainNames {
    local f="composite.txt"
    echo "grid inner.b8.ugrid" > $f
    echo "mapbc inner.mapbc" >> $f
    echo "domain Inner" >> $f
    echo "grid outer.b8.ugrid" >> $f
    echo "mapbc outer.mapbc" >> $f
    echo "domain Outer" >> $f
    echo "grid wake.b8.ugrid" >> $f
    echo "mapbc wake.mapbc" >> $f
    echo "domain Wake" >> $f
}

function createCompositeInputWithoutDomainNames {
    local f="composite.txt"
    echo "grid inner.b8.ugrid" > $f
    echo "mapbc inner.mapbc" >> $f
    echo "grid outer.b8.ugrid" >> $f
    echo "mapbc outer.mapbc" >> $f
    echo "grid wake.b8.ugrid" >> $f
    echo "mapbc wake.mapbc" >> $f
}

@test "if domain name is present in composite recipe, override bc-names according to fun3d custom" {
    run createMapbcFiles
    run createCompositeInputWithDomainNames

    run yoga make-composite --mapbc-only --file composite.txt
    assert_exist composite.mapbc
    run cat composite.mapbc
    assert_output -p "Inner_"
    refute_output -p "aft"
    rm *.mapbc
    rm composite.txt
}

@test "do not modify bc names if domain name is not present" {
    run createMapbcFiles
    run createCompositeInputWithoutDomainNames
    run yoga make-composite --mapbc-only --file composite.txt
    assert_exist composite.mapbc
    run cat composite.mapbc
    assert_output -p "aft"
    refute_output -p "Inner_"
    rm *.mapbc
    rm composite.txt
}

function count_instances {
    local file=$1
    local word=$2
    cat $file | grep $word | wc -l
}

@test "lumping boundary conditions removes duplicates" {
    run createMapbcFiles
    run createCompositeInputWithoutDomainNames
    run yoga make-composite --mapbc-only --file composite.txt --lump-bcs
    assert_exist composite.mapbc

    run count_instances composite.mapbc fore
    assert_output -p "1"
    run count_instances composite.mapbc outer_overset
    assert_output -p "1"

    rm *.mapbc
    rm composite.txt
}
