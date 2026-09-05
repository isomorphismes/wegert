#!/usr/bin/env bash
set -Eeuo pipefail

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
input=${1:-"$repo_root/build/direct/wegert-direct-unsigned.apk"}
output=${2:-"$repo_root/build/direct/wegert-direct-debug.apk"}
android_home=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
build_tools=${ANDROID_BUILD_TOOLS:-}

[[ -f $input ]] || { echo "missing unsigned APK: $input" >&2; exit 1; }
[[ -f $repo_root/app/wegert-debug.keystore ]] || { echo "missing public debug keystore" >&2; exit 1; }
[[ -n $android_home ]] || { echo "ANDROID_HOME/ANDROID_SDK_ROOT is required" >&2; exit 1; }
if [[ -z $build_tools ]]; then
  build_tools=$(find "$android_home/build-tools" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)
fi
apksigner="$build_tools/apksigner"
[[ -x $apksigner ]] || { echo "apksigner not found" >&2; exit 1; }

"$apksigner" sign \
  --ks "$repo_root/app/wegert-debug.keystore" \
  --ks-type PKCS12 \
  --ks-pass pass:wegert-debug \
  --key-pass pass:wegert-debug \
  --ks-key-alias wegert-debug \
  --out "$output" \
  "$input"
"$apksigner" verify --verbose "$output"
