#!/bin/bash

function run_performance_test {
    local ranks=$1
    local case_name=$2
    local script=$3
    local bcs=$4
    local outfile="${case_name}.dat"
    $MPI_COMMAND -np $ranks yoga assemble --script $script --bc $bcs > screen.output
    local timing_line=$(grep 'Total assembly time:' screen.output)
    local node_type_line=$(grep "Yoga:" screen.output | grep orphan)
    local s="Ranks: ${ranks} $timing_line"
    echo $s
    echo $s >> $outfile
    echo $node_type_line
    echo $node_type_line >> $outfile
}

function run_sweep {
    local ranks=($1)
    local case=$2
    echo "Running:                 $case"
    echo "with assembly script:    $3"
    echo "and boundary conditions: $4"
    echo "output file:             $case.dat"
    rm -f $case.dat
    for i in "${ranks[@]}"
    do
        run_performance_test $i $case $3 $4
    done
}







