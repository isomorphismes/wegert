#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -Eeuo pipefail

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
# shellcheck disable=SC1091
source "$repo_root/fdroid/release-values.sh"

classes_dex=${1:-"$repo_root/build/direct/classes.dex"}
native_root=${2:-"$repo_root/build/direct/native"}
output=${3:-"$repo_root/build/direct/wegert-direct-unsigned.apk"}
android_home=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
build_tools=${ANDROID_BUILD_TOOLS:-}

[[ -f $classes_dex ]] || { echo "missing direct classes.dex: $classes_dex" >&2; exit 1; }
[[ -n $android_home ]] || { echo "ANDROID_HOME/ANDROID_SDK_ROOT is required" >&2; exit 1; }
if [[ -z $build_tools ]]; then
  build_tools=$(find "$android_home/build-tools" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)
fi
[[ -d $build_tools ]] || { echo "Android build-tools not found" >&2; exit 1; }

aapt2="$build_tools/aapt2"
zipalign="$build_tools/zipalign"
android_jar="$android_home/platforms/android-36/android.jar"
for required in "$aapt2" "$zipalign" "$android_jar"; do
  [[ -e $required ]] || { echo "missing Android packaging input: $required" >&2; exit 1; }
done
command -v zip >/dev/null 2>&1 || { echo "zip not found" >&2; exit 1; }

work="$repo_root/build/direct/apk-work"
rm -rf "$work"
mkdir -p "$work/stage/assets/licenses" "$(dirname -- "$output")"

bash "$repo_root/android-direct/assemble-shader.sh" "$work/stage/assets/wegert.frag"
cp "$repo_root/LICENSE" "$work/stage/assets/licenses/WEGERT_LICENSE.txt"
cp "$repo_root/NOTICE" "$work/stage/assets/licenses/NOTICE.txt"

"$aapt2" compile --dir "$repo_root/app/src/main/res" -o "$work/resources.zip"
"$aapt2" link \
  -I "$android_jar" \
  --manifest "$repo_root/android-direct/AndroidManifest.xml" \
  --min-sdk-version 26 \
  --target-sdk-version 36 \
  --version-code "$WEGERT_VERSION_CODE" \
  --version-name "$WEGERT_VERSION_NAME" \
  -R "$work/resources.zip" \
  -o "$work/base.apk"

cp "$work/base.apk" "$work/unaligned.apk"
cp "$classes_dex" "$work/stage/classes.dex"
for abi in arm64-v8a armeabi-v7a x86_64; do
  library="$native_root/$abi/libwegert.so"
  [[ -f $library ]] || { echo "missing native library: $library" >&2; exit 1; }
  mkdir -p "$work/stage/lib/$abi"
  cp "$library" "$work/stage/lib/$abi/libwegert.so"
done
(
  cd "$work/stage"
  zip -q -u "$work/unaligned.apk" \
    classes.dex \
    assets/wegert.frag \
    assets/licenses/WEGERT_LICENSE.txt \
    assets/licenses/NOTICE.txt \
    lib/arm64-v8a/libwegert.so \
    lib/armeabi-v7a/libwegert.so \
    lib/x86_64/libwegert.so
)
"$zipalign" -f -p 4 "$work/unaligned.apk" "$output"
