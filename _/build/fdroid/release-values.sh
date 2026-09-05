#!/usr/bin/env bash

# Source this file from F-Droid scripts. Release identity and Android tool
# versions live in a plain properties file so F-Droid never needs Gradle to
# discover or build a release.

release_values_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
release_values_file="$release_values_root/fdroid/release.properties"

release_value() {
    local key="$1"
    sed -n "s/^${key}=//p" "$release_values_file"
}

WEGERT_VERSION_NAME="$(release_value versionName)"
WEGERT_VERSION_CODE="$(release_value versionCode)"
WEGERT_MIN_SDK="$(release_value minSdk)"
WEGERT_TARGET_SDK="$(release_value targetSdk)"
WEGERT_BUILD_TOOLS="$(release_value buildTools)"
WEGERT_CMAKE_VERSION="$(release_value cmake)"
WEGERT_NDK_VERSION="$(release_value ndk)"

test -n "$WEGERT_VERSION_NAME"
test -n "$WEGERT_VERSION_CODE"
test -n "$WEGERT_MIN_SDK"
test -n "$WEGERT_TARGET_SDK"
test -n "$WEGERT_BUILD_TOOLS"
test -n "$WEGERT_CMAKE_VERSION"
test -n "$WEGERT_NDK_VERSION"
[[ "$WEGERT_VERSION_NAME" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
[[ "$WEGERT_VERSION_CODE" =~ ^[0-9]+$ ]]
[[ "$WEGERT_MIN_SDK" =~ ^[0-9]+$ ]]
[[ "$WEGERT_TARGET_SDK" =~ ^[0-9]+$ ]]

export WEGERT_VERSION_NAME WEGERT_VERSION_CODE
export WEGERT_MIN_SDK WEGERT_TARGET_SDK
export WEGERT_BUILD_TOOLS WEGERT_CMAKE_VERSION WEGERT_NDK_VERSION
