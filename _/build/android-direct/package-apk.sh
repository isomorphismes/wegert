#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -Eeuo pipefail

build_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
git_root=$(git -C "$build_root" rev-parse --show-toplevel)
# shellcheck disable=SC1091
source "$build_root/fdroid/release-values.sh"

classes_dex=${1:-"$build_root/build/direct/classes.dex"}
native_root=${2:-"$build_root/build/direct/native"}
output=${3:-"$build_root/build/direct/wegert-direct-unsigned.apk"}
android_home=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
build_tools=${ANDROID_BUILD_TOOLS:-}
source_date_epoch=${SOURCE_DATE_EPOCH:-$(git -C "$git_root" show -s --format=%ct HEAD)}

[[ -f $classes_dex ]] || { echo "missing direct classes.dex: $classes_dex" >&2; exit 1; }
[[ $source_date_epoch =~ ^[0-9]+$ ]] || { echo "invalid SOURCE_DATE_EPOCH: $source_date_epoch" >&2; exit 1; }
[[ -n $android_home ]] || { echo "ANDROID_HOME/ANDROID_SDK_ROOT is required" >&2; exit 1; }
if [[ -z $build_tools ]]; then
  build_tools=$(find "$android_home/build-tools" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)
fi
[[ -d $build_tools ]] || { echo "Android build-tools not found" >&2; exit 1; }

aapt2="$build_tools/aapt2"
zipalign="$build_tools/zipalign"
android_jar="$android_home/platforms/android-$WEGERT_TARGET_SDK/android.jar"
for required in "$aapt2" "$zipalign" "$android_jar"; do
  [[ -e $required ]] || { echo "missing Android packaging input: $required" >&2; exit 1; }
done
command -v zip >/dev/null 2>&1 || { echo "zip not found" >&2; exit 1; }

work="$build_root/build/direct/apk-work"
rm -rf "$work"
mkdir -p "$work/stage/assets/licenses" "$(dirname -- "$output")"

bash "$build_root/android-direct/assemble-shader.sh" "$work/stage/assets/wegert.frag"
cp "$git_root/LICENSE" "$work/stage/assets/licenses/WEGERT_LICENSE.txt"
cp "$git_root/NOTICE" "$work/stage/assets/licenses/NOTICE.txt"

"$aapt2" compile --dir "$build_root/app/src/main/res" -o "$work/resources.zip"
"$aapt2" link \
  -I "$android_jar" \
  --manifest "$build_root/android-direct/AndroidManifest.xml" \
  --min-sdk-version "$WEGERT_MIN_SDK" \
  --target-sdk-version "$WEGERT_TARGET_SDK" \
  --version-code "$WEGERT_VERSION_CODE" \
  --version-name "$WEGERT_VERSION_NAME" \
  -o "$work/base.apk" \
  "$work/resources.zip"

cp "$work/base.apk" "$work/unaligned.apk"
cp "$classes_dex" "$work/stage/classes.dex"
for abi in arm64-v8a armeabi-v7a x86_64; do
  library="$native_root/$abi/libwegert.so"
  [[ -f $library ]] || { echo "missing native library: $library" >&2; exit 1; }
  mkdir -p "$work/stage/lib/$abi"
  cp "$library" "$work/stage/lib/$abi/libwegert.so"
done

python3 - "$source_date_epoch" "$work/stage" <<'PY'
import os
import sys
from pathlib import Path

epoch = int(sys.argv[1])
root = Path(sys.argv[2])
for path in root.rglob("*"):
    if path.is_file():
        os.utime(path, (epoch, epoch))
PY

(
  cd "$work/stage"
  zip -q -X -u "$work/unaligned.apk" \
    classes.dex \
    assets/wegert.frag \
    assets/licenses/WEGERT_LICENSE.txt \
    assets/licenses/NOTICE.txt \
    lib/arm64-v8a/libwegert.so \
    lib/armeabi-v7a/libwegert.so \
    lib/x86_64/libwegert.so
)
"$zipalign" -f -p 4 "$work/unaligned.apk" "$output"
