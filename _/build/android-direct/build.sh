#!/usr/bin/env bash
set -Eeuo pipefail

build_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
classes_dex=${1:-"$build_root/build/direct/classes.dex"}
output=${2:-"$build_root/build/direct/wegert-direct-unsigned.apk"}

bash "$build_root/android-direct/build-native.sh" "$build_root/build/direct/native"
bash "$build_root/android-direct/package-apk.sh" \
  "$classes_dex" "$build_root/build/direct/native" "$output"
bash "$build_root/android-direct/verify-apk.sh" "$output"
