#!/bin/bash -e

setup() {
    source test/setup.bats
}

@test "list currently loaded extensions" {
    run yoga extensions --list
    assert_output -p "Loaded extensions:"
    assert_output -p "core"
}

@test "use a different settings file location" {
  run yoga extensions --load experimental
  # Make sure that the current working directory is where the file gets written
  # because we set INF_SETTINGS_DIR in the test fixture inside setup.bats
  # which lives in one-ring/bats
  run cat ".infinity/yoga_extensions.json"
  assert_output -p "experimental"
}
