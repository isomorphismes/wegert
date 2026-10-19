#!/usr/bin/env bash
set -euo pipefail
# F-Droid may explicitly supply NDK_ROOT. Ignore runner-wide Android NDK
# defaults: merely installing the declared version does not select it.
[[ $# -eq 2 ]] || { echo 'usage: select-ndk.sh SDK_ROOT EXPECTED_REVISION' >&2; exit 2; }
sdk_root=$1
expected=$2
[[ $expected =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || exit 2
ndk_root=${NDK_ROOT:-"$sdk_root/ndk/$expected"}
properties="$ndk_root/source.properties"
[[ -s $properties ]] || { echo "NDK version evidence missing: $properties" >&2; exit 1; }
observed=$(awk -F= '/^[[:space:]]*Pkg[.]Revision[[:space:]]*=/ {
    value=$2; gsub(/[[:space:]]/, "", value); count++
} END { if (count != 1) exit 1; print value }' "$properties") || {
    echo "NDK version evidence must contain exactly one Pkg.Revision: $properties" >&2; exit 1;
}
if [[ $observed != "$expected" ]]; then
    echo "NDK version mismatch: required $expected, observed $observed at $ndk_root" >&2
    exit 1
fi
[[ -s $ndk_root/build/cmake/android.toolchain.cmake ]] || {
    echo "NDK CMake toolchain missing at $ndk_root" >&2; exit 1;
}
printf 'F-Droid NDK: %s at %s\n' "$observed" "$ndk_root" >&2
printf '%s\n' "$ndk_root"
