#!/usr/bin/env bash
set -Eeuo pipefail

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output_root=${1:-"$repo_root/build/direct/native"}
android_home=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}
ndk=${ANDROID_NDK_HOME:-}

if [[ -z $ndk && -n $android_home ]]; then
  ndk="$android_home/ndk/29.0.14206865"
fi
[[ -d $ndk ]] || { echo "Android NDK r29 not found; set ANDROID_NDK_HOME" >&2; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "cmake not found" >&2; exit 1; }

rm -rf "$output_root"
mkdir -p "$output_root"

for abi in arm64-v8a armeabi-v7a x86_64; do
  build_dir="$repo_root/build/direct/cmake/$abi"
  rm -rf "$build_dir"
  cmake -S "$repo_root" -B "$build_dir" \
    -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$abi" \
    -DANDROID_PLATFORM=android-26 \
    -DANDROID_STL=none \
    -DWEGERT_USE_ICK_PREBUILT=OFF \
    -DCMAKE_BUILD_TYPE=Release
  cmake --build "$build_dir" --target wegert --parallel 2
  library="$build_dir/libwegert.so"
  [[ -f $library ]] || { echo "missing native library for $abi: $library" >&2; exit 1; }
  mkdir -p "$output_root/$abi"
  cp "$library" "$output_root/$abi/libwegert.so"
done
