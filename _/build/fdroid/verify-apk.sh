#!/usr/bin/env bash
set -euo pipefail

if [[ "$#" -ne 1 ]]; then
    echo "usage: $0 APK" >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$repo_root/fdroid/release-values.sh"

apk="$1"
bash "$repo_root/fdroid/verify-unsigned.sh" "$apk"
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

sdk_root="${SDK_ROOT:-${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}}"
if [[ -z "$sdk_root" ]]; then
    echo "SDK_ROOT, ANDROID_SDK_ROOT or ANDROID_HOME is required" >&2
    exit 1
fi
aapt="$sdk_root/build-tools/$WEGERT_BUILD_TOOLS/aapt"
zipalign="$sdk_root/build-tools/$WEGERT_BUILD_TOOLS/zipalign"
test -x "$aapt"
test -x "$zipalign"
"$aapt" dump badging "$apk" > "$scratch/badging"
grep -Fq "package: name='org.isomorphisms.wegert' versionCode='$WEGERT_VERSION_CODE' versionName='$WEGERT_VERSION_NAME'" "$scratch/badging"
grep -Fxq "application-label:'zero & infinity'" "$scratch/badging"
grep -Fxq "sdkVersion:'$WEGERT_MIN_SDK'" "$scratch/badging"
grep -Fxq "targetSdkVersion:'$WEGERT_TARGET_SDK'" "$scratch/badging"
grep -Fq "launchable-activity: name='android.app.NativeActivity'" "$scratch/badging"
if grep -Eq '^classes([0-9]*)[.]dex$' "$scratch/files"; then
    echo "NativeActivity F-Droid APK unexpectedly contains DEX" >&2
    exit 1
fi
if grep -Fq "android.permission.INTERNET" "$scratch/badging"; then
    echo "Offline Wegert release unexpectedly requests network permission" >&2
    exit 1
fi
if grep -Eq '^application-(debuggable|testOnly)' "$scratch/badging"; then
    echo 'F-Droid APK contains a debug/test-only application flag' >&2; exit 1
fi
"$zipalign" -c -P 16 4 "$apk"
sha256sum "$apk"
