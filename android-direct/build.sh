#!/usr/bin/env bash
set -Eeuo pipefail

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
classes_dex=${1:-"$repo_root/build/direct/classes.dex"}
output=${2:-"$repo_root/build/direct/wegert-direct-unsigned.apk"}

"$repo_root/android-direct/build-native.sh" "$repo_root/build/direct/native"
"$repo_root/android-direct/package-apk.sh" \
  "$classes_dex" "$repo_root/build/direct/native" "$output"
"$repo_root/android-direct/verify-apk.sh" "$output"
