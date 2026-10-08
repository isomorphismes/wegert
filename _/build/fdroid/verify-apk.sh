#!/usr/bin/env bash
set -euo pipefail

if [[ "$#" -ne 1 ]]; then
    echo "usage: $0 APK" >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"

apk="$1"
test -s "$apk"
unzip -t "$apk" >/dev/null

scratch="$(mktemp -d "${TMPDIR:-/tmp}/wegert-apk.XXXXXX")"
trap 'rm -rf "$scratch"' EXIT
unzip -Z1 "$apk" > "$scratch/files"

for library in \
    lib/arm64-v8a/libwegert.so \
    lib/armeabi-v7a/libwegert.so \
    lib/x86_64/libwegert.so; do
    grep -Fxq "$library" "$scratch/files"
done

sed -n 's,^lib/\([^/][^/]*\)/.*$,\1,p' "$scratch/files" | sort -u > "$scratch/abis"
printf '%s\n' arm64-v8a armeabi-v7a x86_64 | sort > "$scratch/expected-abis"
cmp "$scratch/expected-abis" "$scratch/abis"

sdk_root="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [[ -z "$sdk_root" ]]; then
    echo "ANDROID_SDK_ROOT or ANDROID_HOME is required" >&2
    exit 1
fi
aapt="$sdk_root/build-tools/$WEGERT_BUILD_TOOLS/aapt"
apksigner="$sdk_root/build-tools/$WEGERT_BUILD_TOOLS/apksigner"
test -x "$aapt"
test -x "$apksigner"
"$aapt" dump badging "$apk" > "$scratch/badging"
grep -Fq "package: name='org.isomorphisms.wegert' versionCode='$WEGERT_VERSION_CODE' versionName='$WEGERT_VERSION_NAME'" "$scratch/badging"
grep -Fq "application-label:'zero & infinity'" "$scratch/badging"
grep -Fxq "sdkVersion:'$WEGERT_MIN_SDK'" "$scratch/badging"
grep -Fxq "targetSdkVersion:'$WEGERT_TARGET_SDK'" "$scratch/badging"

# F-Droid signs its own source build. No test/release key may leak into
# the unsigned artifact that our repository gives to fdroidserver.
if "$apksigner" verify "$apk" >"$scratch/signature-check" 2>&1; then
    echo "F-Droid build unexpectedly carries an APK signature" >&2
    exit 1
fi
if grep -Eq '^META-INF/[^/]+[.](RSA|DSA|EC|SF)
 "$scratch/files"; then
    echo "F-Droid build unexpectedly carries JAR signing files" >&2
    exit 1
fi

# This is the independent source-built NativeActivity release lane,
# not the direct-DEX test/qualification artifact.
if grep -Eq '^classes([0-9]*)[.]dex
 "$scratch/files"; then
    echo "F-Droid NativeActivity release unexpectedly contains DEX" >&2
    exit 1
fi
if grep -Fq "android.permission.INTERNET" "$scratch/badging"; then
    echo "Unexpected Android network permission in offline Wegert release" >&2
    exit 1
fi

sha256sum "$apk"
