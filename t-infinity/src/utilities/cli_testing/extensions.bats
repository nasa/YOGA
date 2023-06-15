#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "list currently loaded extensions" {
    run inf extensions --list
    assert_output -p "Loaded extensions:"
    assert_output -p "core"
}

@test "list available extensions" {
    run inf extensions --avail
    assert_output -p "Available extensions:"
    assert_output -p "experimental"
}

@test "load all available extensions" {
    run inf extensions --avail
    assert_output -p "Available extensions:"
    assert_output -p "experimental"
    run inf extensions --load-all
    run inf extensions --list
    assert_output -p "experimental"
}

@test "don't load unrecognized extensions" {
    run inf extensions --load fake
    assert_output -p "No extension"
    assert_output -p "fake"

    run inf extensions --list
    refute_output -p "fake"
}

@test "load and unload extensions" {
    run inf extensions --load experimental
    assert_output -p "Loaded"
    assert_output -p "experimental"

    run inf extensions --list
    assert_output -p "experimental"

    run inf extensions --unload experimental
    assert_output -p "Unloaded"
    assert_output -p "experimental"

    run inf extensions --list
    refute_output -p "experimental"
}

@test "purge extensions" {
    run inf extensions --load experimental
    run inf extensions --purge
    run inf extensions --list
    refute_output -p "experimental"
}

@test "make sure extensions actually load" {
    run inf --help
    refute_output -p "analyze-trace"

    run inf extensions --load devtools
    run inf --help
    assert_output -p "analyze-trace"

}

@test "use a different settings file location" {
  run inf extensions --load devtools
  # Make sure that the current working directory is where the file gets written
  # because we set INF_SETTINGS_DIR in the test fixture inside setup.bats
  # which lives in one-ring/bats
  run cat ".infinity/inf_extensions.json"
  assert_output -p "devtools"
}
