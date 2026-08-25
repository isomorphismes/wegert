#!/usr/bin/env bash

# Source this file from F-Droid scripts. It reads the release identity from the
# same source-controlled declarations that Gradle uses.

release_values_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
release_values_gradle="$release_values_root/app/build.gradle.kts"

WEGERT_VERSION_NAME="$(sed -n 's/^val releaseVersionName = "\([^"]*\)"$/\1/p' "$release_values_gradle")"
WEGERT_VERSION_CODE="$(sed -n 's/^val releaseVersionCode = \([0-9][0-9]*\)$/\1/p' "$release_values_gradle")"

test -n "$WEGERT_VERSION_NAME"
test -n "$WEGERT_VERSION_CODE"
[[ "$WEGERT_VERSION_NAME" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
[[ "$WEGERT_VERSION_CODE" =~ ^[0-9]+$ ]]

export WEGERT_VERSION_NAME WEGERT_VERSION_CODE
