#!/usr/bin/env bash

# Source this file from Android/F-Droid scripts. release.properties is the
# single handwritten authority for package/version and Android tool versions.
release_values_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
release_values_file="$release_values_root/fdroid/release.properties"

release_value() {
    local key="$1"
    sed -n "s/^${key}=//p" "$release_values_file"
}

WEGERT_PACKAGE_ID="$(release_value packageId)"
WEGERT_VERSION_NAME="$(release_value versionName)"
WEGERT_VERSION_CODE="$(release_value versionCode)"
WEGERT_MIN_SDK="$(release_value minSdk)"
WEGERT_COMPILE_SDK="$(release_value compileSdk)"
WEGERT_TARGET_SDK="$(release_value targetSdk)"
WEGERT_BUILD_TOOLS="$(release_value buildTools)"
WEGERT_CMAKE_VERSION="$(release_value cmake)"
WEGERT_NDK_VERSION="$(release_value ndk)"
WEGERT_RELEASE_TAG="v$WEGERT_VERSION_NAME"

for value in     WEGERT_PACKAGE_ID WEGERT_VERSION_NAME WEGERT_VERSION_CODE     WEGERT_MIN_SDK WEGERT_COMPILE_SDK WEGERT_TARGET_SDK     WEGERT_BUILD_TOOLS WEGERT_CMAKE_VERSION WEGERT_NDK_VERSION; do
    test -n "${!value}"
done

[[ "$WEGERT_PACKAGE_ID" =~ ^[A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+$ ]]
[[ "$WEGERT_VERSION_NAME" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
[[ "$WEGERT_VERSION_CODE" =~ ^[0-9]+$ ]]
[[ "$WEGERT_MIN_SDK" =~ ^[0-9]+$ ]]
[[ "$WEGERT_COMPILE_SDK" =~ ^[0-9]+$ ]]
[[ "$WEGERT_TARGET_SDK" =~ ^[0-9]+$ ]]

export WEGERT_PACKAGE_ID WEGERT_VERSION_NAME WEGERT_VERSION_CODE WEGERT_RELEASE_TAG
export WEGERT_MIN_SDK WEGERT_COMPILE_SDK WEGERT_TARGET_SDK
export WEGERT_BUILD_TOOLS WEGERT_CMAKE_VERSION WEGERT_NDK_VERSION
