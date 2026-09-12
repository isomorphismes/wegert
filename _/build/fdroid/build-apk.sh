#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"

sdk_root="${SDK_ROOT:-${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}}"
if [[ -z "$sdk_root" ]]; then
    echo "SDK_ROOT, ANDROID_SDK_ROOT, or ANDROID_HOME is required" >&2
    exit 1
fi

ndk_root="${NDK_ROOT:-${ANDROID_NDK_HOME:-${ANDROID_NDK:-$sdk_root/ndk/$WEGERT_NDK_VERSION}}}"
output="${1:-$repo_root/build/fdroid/wegert-release-unsigned.apk}"
mkdir -p "$(dirname "$output")"
output="$(cd "$(dirname "$output")" && pwd)/$(basename "$output")"

build_tools="$sdk_root/build-tools/$WEGERT_BUILD_TOOLS"
aapt2="$build_tools/aapt2"
zipalign="$build_tools/zipalign"
android_jar="$sdk_root/platforms/android-$WEGERT_TARGET_SDK/android.jar"
cmake="$sdk_root/cmake/$WEGERT_CMAKE_VERSION/bin/cmake"
ninja="$sdk_root/cmake/$WEGERT_CMAKE_VERSION/bin/ninja"
toolchain="$ndk_root/build/cmake/android.toolchain.cmake"

for required in "$aapt2" "$zipalign" "$cmake" "$ninja"; do
    test -x "$required"
done
test -s "$android_jar"
test -s "$toolchain"
command -v python3 >/dev/null

source_date_epoch="${SOURCE_DATE_EPOCH:-$(git -C "$repo_root" show -s --format=%ct HEAD)}"
[[ "$source_date_epoch" =~ ^[0-9]+$ ]]

work="$repo_root/build/fdroid/manual"
rm -rf "$work"
mkdir -p "$work/native" "$work/package/lib" "$work/assets" "$work/res"

# Gradle used to assemble this one generated asset. Keep the exact marker
# contract while doing the replacement directly.
python3 - "$repo_root/wegert.frag.in" "$repo_root/wegert_color.glsl" "$work/assets/wegert.frag" <<'PY'
from pathlib import Path
import sys

template = Path(sys.argv[1]).read_text(encoding="utf-8")
color_core = Path(sys.argv[2]).read_text(encoding="utf-8")
marker = "/*__WEGERT_COLOR_CORE__*/"
if template.count(marker) != 1:
    raise SystemExit("Wegert fragment template must contain exactly one coloring-core marker")
Path(sys.argv[3]).write_text(template.replace(marker, color_core), encoding="utf-8")
PY

# aapt2 needs the package name and a concrete XML label rather than AGP's
# manifest placeholder substitution.
python3 - "$repo_root/app/src/main/AndroidManifest.xml" "$work/AndroidManifest.xml" <<'PY'
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text(encoding="utf-8")
if source.count("${appLabel}") != 1:
    raise SystemExit("AndroidManifest.xml must contain exactly one ${appLabel} placeholder")
if '<manifest xmlns:android="http://schemas.android.com/apk/res/android">' not in source:
    raise SystemExit("unexpected AndroidManifest.xml root element")
source = source.replace(
    '<manifest xmlns:android="http://schemas.android.com/apk/res/android">',
    '<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="org.isomorphisms.wegert">',
    1,
)
source = source.replace("${appLabel}", "zero &amp; infinity")
Path(sys.argv[2]).write_text(source, encoding="utf-8")
PY

cp -a "$repo_root/app/src/main/res/." "$work/res/"
find "$work/assets" "$work/res" -type f -exec touch -d "@$source_date_epoch" {} +
touch -d "@$source_date_epoch" "$work/AndroidManifest.xml"

readonly abis=(arm64-v8a armeabi-v7a x86_64)
for abi in "${abis[@]}"; do
    native_build="$work/native/$abi"
    "$cmake" -S "$repo_root" -B "$native_build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_MAKE_PROGRAM="$ninja" \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
        -DANDROID_ABI="$abi" \
        -DANDROID_PLATFORM="android-$WEGERT_MIN_SDK" \
        -DANDROID_STL=none \
        -DWEGERT_USE_ICK_PREBUILT=OFF \
        -DCMAKE_C_FLAGS="-ffile-prefix-map=$repo_root=. -ffile-prefix-map=$ndk_root=/opt/android-ndk"
    "$cmake" --build "$native_build" --target wegert

    library="$native_build/libwegert.so"
    test -s "$library"
    mkdir -p "$work/package/lib/$abi"
    cp "$library" "$work/package/lib/$abi/libwegert.so"
done
find "$work/package" -type f -exec touch -d "@$source_date_epoch" {} +

"$aapt2" compile --dir "$work/res" -o "$work/resources.zip"
"$aapt2" link \
    -I "$android_jar" \
    --manifest "$work/AndroidManifest.xml" \
    --min-sdk-version "$WEGERT_MIN_SDK" \
    --target-sdk-version "$WEGERT_TARGET_SDK" \
    --version-code "$WEGERT_VERSION_CODE" \
    --version-name "$WEGERT_VERSION_NAME" \
    -A "$work/assets" \
    -o "$work/base.apk" \
    "$work/resources.zip"

# F-Droid's production build image intentionally has a small userspace and
# does not guarantee the external `zip` command. Append the native libraries
# with Python's standard library, stored (not deflated), with one deterministic
# timestamp so zipalign can page-align them afterwards.
python3 - "$work/base.apk" "$work/package" "$source_date_epoch" <<'PY'
from datetime import datetime, timezone
from pathlib import Path
import sys
import zipfile

apk = Path(sys.argv[1])
package = Path(sys.argv[2])
epoch = int(sys.argv[3])
minimum = int(datetime(1980, 1, 1, tzinfo=timezone.utc).timestamp())
stamp = datetime.fromtimestamp(max(epoch, minimum), tz=timezone.utc)
zip_stamp = (stamp.year, stamp.month, stamp.day, stamp.hour, stamp.minute, stamp.second)

entries = [
    "lib/arm64-v8a/libwegert.so",
    "lib/armeabi-v7a/libwegert.so",
    "lib/x86_64/libwegert.so",
]
with zipfile.ZipFile(apk, mode="a", allowZip64=True) as archive:
    for name in entries:
        source = package / name
        if not source.is_file():
            raise SystemExit(f"missing native library: {source}")
        info = zipfile.ZipInfo(name, date_time=zip_stamp)
        info.compress_type = zipfile.ZIP_STORED
        info.create_system = 3
        info.external_attr = 0o100644 << 16
        archive.writestr(info, source.read_bytes())
PY

rm -f "$output"
"$zipalign" -P 16 -f 4 "$work/base.apk" "$output"
test -s "$output"
printf '%s\n' "$output"
