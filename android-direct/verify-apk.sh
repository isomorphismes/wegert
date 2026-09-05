#!/usr/bin/env bash
set -Eeuo pipefail

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
apk=${1:-"$repo_root/build/direct/wegert-direct-unsigned.apk"}
android_home=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
build_tools=${ANDROID_BUILD_TOOLS:-}

[[ -f $apk ]] || { echo "missing APK: $apk" >&2; exit 1; }
command -v unzip >/dev/null 2>&1 || { echo "unzip not found" >&2; exit 1; }
[[ -n $android_home ]] || { echo "ANDROID_HOME/ANDROID_SDK_ROOT is required" >&2; exit 1; }
if [[ -z $build_tools ]]; then
  build_tools=$(find "$android_home/build-tools" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)
fi
aapt2="$build_tools/aapt2"
[[ -x $aapt2 ]] || { echo "aapt2 not found" >&2; exit 1; }

listing=$(mktemp)
trap 'rm -f "$listing"' EXIT
unzip -l "$apk" > "$listing"
for required in \
  classes.dex \
  assets/wegert.frag \
  lib/arm64-v8a/libwegert.so \
  lib/armeabi-v7a/libwegert.so \
  lib/x86_64/libwegert.so; do
  grep -Fq "$required" "$listing" || { echo "APK missing $required" >&2; exit 1; }
done
[[ $(grep -Ec '[[:space:]]classes\.dex$' "$listing") -eq 1 ]] || {
  echo "APK must contain exactly one classes.dex" >&2
  exit 1
}
! grep -Eq '[.]class$' "$listing"

badging=$("$aapt2" dump badging "$apk")
printf '%s\n' "$badging" | grep -Fq "package: name='org.isomorphisms.wegert'"
printf '%s\n' "$badging" | grep -Fq "launchable-activity: name='org.isomorphisms.wegert.WegertActivity'"

printf 'PASS: direct DEX/JNI APK %s\n' "$apk"
sha256sum "$apk"
