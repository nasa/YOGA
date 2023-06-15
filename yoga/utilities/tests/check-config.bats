#!/bin/bash -e

setup() {
    source test/setup.bats
    good_config="Config file ok."
}


function createValidConfigFile {
    local f="yoga.config"
    echo "component-grid-importance 1 0" > $f
}

function createBadConfig {
    local f="yoga.config"
    echo "some-nonsense 1 0" > $f
}


@test "good config file" {
    createValidConfigFile
    run yoga check-syntax --config yoga.config
    assert_output -p $good_config
}

@test "config with error" {
    createBadConfig
    run yoga check-syntax --config yoga.config
    refute_output -p $good_config
}

